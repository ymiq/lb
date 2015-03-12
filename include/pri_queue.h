
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


/*
 * ==========================================================================================
 *			TEMPLATE: pri_queue
 * ==========================================================================================
 */
template<typename T>
class pri_queue {
public:
	pri_queue();
	~pri_queue();
	
	bool init(int pris, int *qsize);
	bool empty(void);
	bool full(void);
	int size(void);
	bool push(T data, int qos);
	T front(void);
	T pop(void);
	void pop(T ref);
	
protected:
	class lockless_queue{
	public:
		lockless_queue();
		~lockless_queue();
		
		bool init(int size);
		bool empty(void);
		bool full(void);
		int size(void);
		bool push(T data);
		T front(void);
		T pop(void);
		
	protected:
		
	private:
		T *queue;
		unsigned long head_tail;
		unsigned int mod_size;
		unsigned int mod_mask;
	};
	
private:
	lockless_queue *llq;
	int *llq_size;
	int max_pri;
};


template<typename T>
pri_queue<T>::pri_queue() {
	llq = NULL;
	llq_size = NULL;
}


template<typename T>
pri_queue<T>::~pri_queue() {
	if (llq) {
		delete[] llq;
	}
	if (llq_size) {
		delete[] llq_size;
	}
}


template<typename T>
bool pri_queue<T>::init(int pris, int *qsize) {
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


template<typename T>
bool pri_queue<T>::push(T data, int qos) {
	if ((qos >= 0) && (qos < max_pri)) {
		return llq[qos].push(data);
	}
	return false;
}


template<typename T>
T pri_queue<T>::front(void) {
	for (int i=0; i<max_pri; i++) {
		T ret = llq[i].front();
		if (ret) {
			return ret;
		}
	}
	return NULL;
}


template<typename T>
void pri_queue<T>::pop(T ref) {
	for (int i=0; i<max_pri; i++) {
		if (llq[i].front() == ref) {
			llq[i].pop();
			return;
		}
	}
}


template<typename T>
T pri_queue<T>::pop(void) {
	for (int i=0; i<max_pri; i++) {
		T ret = llq[i].pop();
		if (ret) {
			return ret;
		}
	}
	return NULL;
}


template<typename T>
int pri_queue<T>::size(void) {
	int size = 0;
	for (int i=0; i<max_pri; i++) {
		size += llq[i].size();
	}
	return size;
}


template<typename T>
bool pri_queue<T>::empty(void) {
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


template<typename T>
pri_queue<T>::lockless_queue::lockless_queue() {
	queue = NULL;
}


template<typename T>
pri_queue<T>::lockless_queue::~lockless_queue() {
	if (queue) {
		delete[] queue;
	}
}


template<typename T>
bool pri_queue<T>::lockless_queue::init(int size) {
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


template<typename T>
bool pri_queue<T>::lockless_queue::push(T data) {
	register unsigned long tmp = head_tail;
	unsigned int head = (unsigned int)(tmp >> 32);
	unsigned int tail = (unsigned int)(tmp & 0xffffffffu);
	unsigned int size = tail - head;
	
	/* 检查Queue是否已满 */
	if (size == mod_size) {
		return false;
	}
	
	/* 获取返回值 */
	tail &= mod_mask;
	queue[tail] = data;
	
	/* 检查避免溢出，增加计数器 */
	if ((tmp & 0x8000000080000000ul) == 0x8000000080000000ul) {
		__sync_fetch_and_sub(&head_tail, 0x8000000080000000ul);
	}
	__sync_fetch_and_add(&head_tail, 0x1ul);
	
	return true;
}


template<typename T>
T pri_queue<T>::lockless_queue::front(void) {
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


template<typename T>
T pri_queue<T>::lockless_queue::pop(void) {
	register unsigned long tmp = head_tail;
	unsigned int head = (unsigned int)(tmp >> 32);
	unsigned int tail = (unsigned int)(tmp & 0xffffffffu);
	unsigned int size = tail - head;
	
	/* 检查Queue是否已空 */
	if (!size) {
		return NULL;
	}
	
	/* 获取返回值 */
	head &= mod_mask;
	T ret = queue[head];
	
	/* 增加计数器 */
	__sync_fetch_and_add(&head_tail, 0x100000000ul);
	
	return ret;
}


template<typename T>
int pri_queue<T>::lockless_queue::size(void) {
	register unsigned long tmp = head_tail;
	unsigned int head = (unsigned int)(tmp >> 32);
	unsigned int tail = (unsigned int)(tmp & 0xffffffffu);
	return (int)(tail - head);
}


template<typename T>
bool pri_queue<T>::lockless_queue::empty(void) {
	register unsigned long tmp = head_tail;
	unsigned int head = (unsigned int)(tmp >> 32);
	unsigned int tail = (unsigned int)(tmp & 0xffffffffu);
	unsigned int size = tail - head;
	return (size == 0) ? true : false;
}


