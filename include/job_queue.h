
/*
 *  线程安全的优先级队列
 *  由于无锁，使用约束较多，暂只用于evsock
 *  用于其他地方请慎重！！！
 */

#include <iostream>
#include <cstdarg>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <log.h>


typedef struct job_node {
	struct job_node *next;
	char *buf;
	size_t len;		
	qao_base *qao;
	bool fragment;
	unsigned int offset;
	serial_data header;
}job_node;


/*
 * ==========================================================================================
 *			TEMPLATE: pri_queue
 * ==========================================================================================
 */
class pri_queue {
public:
	pri_queue();
	~pri_queue();
	
	bool init(int pris, int *qsize);
	bool empty(void);
	bool full(void);
	int size(void);
	bool push(job_node *data, int qos);
	job_node *front(void);
	job_node *pop(void);
	void pop(job_node *ref);
	
protected:
	class lockless_queue{
	public:
		lockless_queue();
		~lockless_queue();
		
		bool init(int size);
		bool empty(void);
		bool full(void);
		int size(void);
		bool push(job_node *data);
		job_node *front(void);
		job_node *pop(void);
		
	protected:
		
	private:
		job_node *head;
		job_node *tail;
		unsigned int size;
		unsigned int mod_size;
		unsigned int mod_mask;
	};
	
private:
	lockless_queue *llq;
	int *llq_size;
	int max_pri;
};


pri_queue::pri_queue() {
	llq = NULL;
	llq_size = NULL;
}


pri_queue::~pri_queue() {
	if (llq) {
		delete[] llq;
	}
	if (llq_size) {
		delete[] llq_size;
	}
}


bool pri_queue::init(int pris, int *qsize) {
	/* 参数检查 */
	if ((pris <= 0) || !qsize) {
		return false;
	}
	
	/* 变量初始化 */
	llq_size = new int[pris];
	llq = new lockless_queue[pris];
	max_pri = pris;
	for (int i=0; i<pris; i++) {
		int size = llq_size[i] = qsize[i];
		if ((llq_size[i] <= 0) || !llq[i].init(size)) {
			delete llq;
			delete llq_size;
			llq = NULL;
			llq_size = NULL;
			return false;
		}
	}
		
	return true;
}


bool pri_queue::push(job_node *data, int qos) {
	if ((qos >= 0) && (qos < max_pri)) {
		return llq[qos].push(data);
	}
	return false;
}


job_node *pri_queue::front(void) {
	for (int i=0; i<max_pri; i++) {
		job_node *ret = llq[i].front();
		if (ret) {
			return ret;
		}
	}
	return NULL;
}


void pri_queue::pop(job_node *ref) {
	for (int i=0; i<max_pri; i++) {
		if (llq[i].front() == ref) {
			llq[i].pop();
			return;
		}
	}
}


job_node *pri_queue::pop(void) {
	for (int i=0; i<max_pri; i++) {
		job_node *ret = llq[i].pop();
		if (ret) {
			return ret;
		}
	}
	return NULL;
}


int pri_queue::size(void) {
	int size = 0;
	for (int i=0; i<max_pri; i++) {
		size += llq[i].size();
	}
	return size;
}


bool pri_queue::empty(void) {
	for (int i=0; i<max_pri; i++) {
		if (!llq[i].empty()) {
			return false;
		}
	}
	return true;
}



/*
 * ==========================================================================================
 *			TEMPLATE: lockless_queue
 * ==========================================================================================
 */


pri_queue::lockless_queue::lockless_queue() {
	queue = NULL;
}


pri_queue::lockless_queue::~lockless_queue() {
	if (queue) {
		delete[] queue;
	}
}


bool pri_queue::lockless_queue::init(int size) {
	if (size <= 0) {
		return false;
	}

	/* 调整参数size为2^n */
	if (size >= 0x100000) {
		mod_size = 0x100000u;
	} else {
		int bit = 30;
		for (bit=30;  bit>=0; bit--) {
			if (size & (1u << bit)) {
				break;
			}
		}
		mod_size = 1u << (bit + 1);
	}
	mod_mask = mod_size - 1;
		
	/* 申请优先级队列 */
	queue = new T[mod_size];
	head_tail = 0;
	return true;
}


bool pri_queue::lockless_queue::push(job_node *data) {
	do {
		unsigned long old_val = head_tail;
		unsigned int head = (unsigned int)(tmp >> 32);
		unsigned int tail = (unsigned int)(tmp & 0xffffffffu);
		unsigned int size = tail - head;
		
		/* 检查Queue是否已满 */
		if (size == mod_size - CFG_MAX_CONCURRENCE) {
			return false;
		}
		
		/* 获取写入位置 */
		tail &= mod_mask;
		queue[tail] = data;
		
		/* 检查避免溢出，增加计数器 */
		unsigned long new_val = old_val + 1ul;
		if ((new_val & 0x8000000080000000ul) == 0x8000000080000000ul) {
			new_val &= ~0x8000000080000000ul;
		}
	} while (__sync_val_compare_and_swap(&head_tail, old_val, new_val));
	
	return true;
}


job_node *pri_queue::lockless_queue::front(void) {
	register unsigned long tmp = head_tail;
	unsigned int head = (unsigned int)(tmp >> 32);
	unsigned int tail = (unsigned int)(tmp & 0xffffffffu);
	unsigned int size = tail - head;
	
	/* 检查Queue是否已空 */
	if (!size) {
		return NULL;
	}
	head &= mod_mask;
	return queue[head];
}


job_node *pri_queue::lockless_queue::pop(void) {
	job_node *ret = NULL;
	do {
		unsigned long old_val = head_tail;
		unsigned int head = (unsigned int)(tmp >> 32);
		unsigned int tail = (unsigned int)(tmp & 0xffffffffu);
		unsigned int size = tail - head;
		
		/* 检查Queue是否已空 */
		if (!size) {
			return NULL;
		}
		
		/* 获取返回值 */
		head &= mod_mask;
		ret = queue[head];
		
		/* 更新标记 */
		unsigned long new_val = old_val + 0x100000000ul;
		
	} while (__sync_val_compare_and_swap(&head_tail, old_val, new_val));
		
	return ret;
}


int pri_queue::lockless_queue::size(void) {
	register unsigned long tmp = head_tail;
	unsigned int head = (unsigned int)(tmp >> 32);
	unsigned int tail = (unsigned int)(tmp & 0xffffffffu);
	return (int)(tail - head);
}


bool pri_queue::lockless_queue::empty(void) {
	register unsigned long tmp = head_tail;
	unsigned int head = (unsigned int)(tmp >> 32);
	unsigned int tail = (unsigned int)(tmp & 0xffffffffu);
	unsigned int size = tail - head;
	return (size == 0) ? true : false;
}


