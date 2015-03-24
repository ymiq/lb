/*
 *
 *	无锁动态键值对，用于存储键值对。
 * 	支持多个读者，多个写者。不需要使用RCU_MAN进行辅助更新处理。
 *  T         : 要求值必须为指针，或者整型数据类型（但要求0值表示不合法数据）。
 *  INDEX_SIZE：设置为MAX_HASHS/8比较合理。MAX_HASHS表示动态数组预计存储最多Hash值数量。
 *  INDEX_SIZE：建议为2^n，如果不是，将会被强制调整。
 *
 *  内存使用约：INDEX_SIZE × sizeof(unsigned long) × 32
 *
 *  注意：该模板未实现拷贝构造函数，创建的对象不能被复制，也不建议对该类实例化对象进行复制。
 *
 */

#ifndef __HASH_PAIR_H__
#define __HASH_PAIR_H__

#include <cstdlib>
#include <cstddef>
#include <config.h>
#include <vector>
#include <rcu_obj.h>

using namespace std;


template<typename T, unsigned int INDEX_SIZE>
class hash_pair {

public:
	hash_pair(const hash_pair &tbl);
	hash_pair();
	~hash_pair();

	void remove(unsigned long hash);
	bool add(unsigned long hash, T &pair);
	T get(unsigned long hash);
	T find(unsigned long hash);
	
	class it {
	public:
	    it(hash_pair *instance = NULL, int end_x=0) 
	    			:instance(instance), pos_x(end_x), pos_y(0), pos_n(0) {}
	    ~it(){};

		T operator*();
		it operator++(int);
		it &operator++();
		it &operator=(const it &rv);
		bool operator!=(const it &rv);
		bool operator==(const it &rv);
		
	    void begin(void);
	    void next(void);
	
	protected:
	private:
		hash_pair *instance;
		int pos_x;
		int pos_y;
		int pos_n;
	};

	it begin(void);
	it &end(void);
		
protected:
	T locate(int &pos_x, int &pos_y, int &pos_n, int inc);
	
private:
	/* 定义为2^n - 1 */
	#define CFG_PAIR_ITEM_SIZE		15

	typedef struct hash_item{
		volatile unsigned long hash;
		volatile T pair;
	}hash_item;
	
	typedef struct hash_index {
		hash_item  items[CFG_PAIR_ITEM_SIZE];
		hash_index *next;
		void *reserve;
	}hash_index;
	
	unsigned int mod_size;
	hash_index *hidx;
	it *end_it;
};


template<typename T, unsigned int INDEX_SIZE>
hash_pair<T, INDEX_SIZE>::hash_pair()
{
	unsigned int size = INDEX_SIZE;
	
	if (sizeof(T) != sizeof(void*)) {
		throw "size of template is too large";
	}
	
	/* 调整参数size为2^n */
	if (size >= 0x40000000) {
		mod_size = 0x40000000;
	} else {
		int bit = 30;
		for (bit=30;  bit>=0; bit--) {
			if (size & (1UL << bit)) {
				break;
			}
		}
		mod_size = 1UL << (bit + 1);
	}
	
	/* 申请哈希表索引 */
	hidx = new hash_index[mod_size]();
	end_it = new it(NULL, mod_size);
}


template<typename T, unsigned int INDEX_SIZE>
hash_pair<T, INDEX_SIZE>::hash_pair(const hash_pair<T, INDEX_SIZE> &tbl) {
	throw "copy construct for hash_pair is disabled";
}


template<typename T, unsigned int INDEX_SIZE>
hash_pair<T, INDEX_SIZE>::~hash_pair() {
	/* 删除所有新申请的索引 */
	for(int x=0; x<mod_size; x++) {
		hash_index *pindex = hidx[x].next;
		while (pindex) {
			hash_index *pnext = pindex->next;
			delete pindex;
			pindex = pnext;
		} 
	}
	
	/* 删除NULL对象和索引表 */	
	delete end_it;
	delete hidx;
}


template<typename T, unsigned int INDEX_SIZE>
T hash_pair<T, INDEX_SIZE>::locate(int &pos_x, int &pos_y, int &pos_n, int inc) {
	int x, y, n;
	unsigned long hash;
	hash_index *pindex;
	
	/* 检查是否EOF */
	if (pos_x == mod_size) {
		return NULL;
	}
	
	/* 参数初始化 */
	x = pos_x;
	y = pos_y;
	n = pos_n;
	if (inc) {
		if (++n >= CFG_PAIR_ITEM_SIZE) {
			n = 0;
			y += 1;
		}
	}		
	
	/* 定位到搜索起点 */
	pindex = &hidx[x];
	for (int idy=0; idy<y; idy++) {
		pindex = pindex->next;
		if (!pindex) {
			if (++x == mod_size) {
				/* 所有索引搜索结束 */
				pos_x = mod_size;
				pos_y = 0;
				pos_n = 0;
				return NULL;
			}
			pindex = &hidx[x];
			y = 0;
			n = 0;
			break;
		}
	}
	
	/* 定位下一个 */
	while(1) {
		do {
			for (int idn=n; idn<CFG_PAIR_ITEM_SIZE; idn++) {
				T ret = pindex->items[idn].pair;
				rmb();
				hash = pindex->items[idn].hash;
				
				/* 当前INDEX搜索结束 */
				if (!hash) {
					goto next;
				} else if ((hash != -1ul) && (hash != 1ul)){
					
					/* 定位到有效条目 */
					pos_x = x;
					pos_y = y;
					pos_n = idn;
					return ret;
				}
			}
			y++;
			pindex = pindex->next;
		} while (pindex);
		
next:
		/* 在下一个索引中进行搜索 */
		if (++x == mod_size) {
			/* 所有索引搜索结束 */
			pos_x = mod_size;
			pos_y = 0;
			pos_n = 0;
			return NULL;
		}
		pindex = &hidx[x];
		y = 0;
		n = 0;
	}
	
	return NULL;
}


template<typename T, unsigned int INDEX_SIZE>
T hash_pair<T, INDEX_SIZE>::find(unsigned long hash) {
	unsigned int index;
	unsigned long save_hash;
	hash_index *pindex;
	
	index = (unsigned int)(hash % mod_size);
	pindex = &hidx[index];
	
	do {
		for (int i=0; i<CFG_PAIR_ITEM_SIZE; i++) {
			T ret = pindex->items[i].pair;
			rmb();
			save_hash = pindex->items[i].hash;
			
			/* 搜索结束 */
			if (!save_hash) {
				return (T)0;
			}
			
			/* 获得匹配 */
			if (save_hash == hash) {
				return ret;
			}
		}
		pindex = pindex->next;
	} while (pindex);
	
	return (T)0;
}


template<typename T, unsigned int INDEX_SIZE>
bool hash_pair<T, INDEX_SIZE>::add(unsigned long hash, T &pair)
{
	unsigned int index;
	unsigned long save_hash;
	hash_index *pindex, *prev;
	
	index = (unsigned int)(hash % mod_size);
	prev = pindex = &hidx[index];
	
	/* 第一轮搜索，匹配是否存在该条目 */
	do {
		for (int i=0; i<CFG_PAIR_ITEM_SIZE; i++) {
			T tmp = pindex->items[i].pair;
			rmb();
			save_hash = pindex->items[i].hash;
			
			/* 搜索结束 */
			if (!save_hash) {
				goto phase2;	
			}
			
			/* 获得匹配 */
			if (save_hash == hash) {
				volatile T *ptwr = &pindex->items[i].pair;
				if (__sync_bool_compare_and_swap(ptwr, tmp, pair)) {
					return true;
				}
				return add(hash, pair);
			}
		}
		pindex = pindex->next;
	} while (pindex);
	
	/* 第二轮搜索，插入/更新条目 */
phase2:
	pindex = prev;
	do {
		for (int i=0; i<CFG_PAIR_ITEM_SIZE; i++) {
			save_hash = pindex->items[i].hash;
			
			if ((!save_hash) || (save_hash == -1ul)) {
				volatile unsigned long *phwr = &pindex->items[i].hash;
				if (__sync_bool_compare_and_swap(phwr, save_hash, 1)) {
					pindex->items[i].pair = pair;
					wmb();
					pindex->items[i].hash = hash;
					return true;
				}
				return add(hash, pair);
			}
		}
		prev = pindex;
		pindex = pindex->next;
	} while (pindex);
	
	/* 申请新的索引项 */
	pindex = new hash_index();
	
	/* 更新条目信息 */
	pindex->items[0].pair = pair;
	pindex->items[0].hash = hash;
	wmb();

	/* 增加内存屏障，确保写入先后顺序 */
	hash_index **pwrite = &prev->next;
	if (__sync_bool_compare_and_swap(pwrite, NULL, pindex)) {
		return true;
	}
	delete pindex;
	return add(hash, pair);
}


template<typename T, unsigned int INDEX_SIZE>
void hash_pair<T, INDEX_SIZE>::remove(unsigned long hash)
{
	unsigned int index;
	unsigned long save_hash;
	hash_index *pindex;
	
	index = (unsigned int)(hash % mod_size);
	pindex = &hidx[index];
	
	do {
		for (int i=0; i<CFG_PAIR_ITEM_SIZE; i++) {
			save_hash = pindex->items[i].hash;
			
			/* 搜索结束 */
			if (!save_hash) {
				return;
			}
			
			/* 获得匹配 */
			if (save_hash == hash) {
				volatile unsigned long *phwr = &pindex->items[i].hash;
				if (__sync_bool_compare_and_swap(phwr, save_hash, -1ul)) {
					return;
				}
				remove(hash);
				return;
			}
		}
		pindex = pindex->next;
	} while (pindex);
	
	return;
}


template<typename T, unsigned int INDEX_SIZE>
typename hash_pair<T, INDEX_SIZE>::it hash_pair<T, INDEX_SIZE>::begin(void) {
	it ret(this);
	ret.begin();
	return ret;
}


template<typename T, unsigned int INDEX_SIZE>
typename hash_pair<T, INDEX_SIZE>::it &hash_pair<T, INDEX_SIZE>::end(void) {
	return *end_it;
}


template<typename T, unsigned int INDEX_SIZE>
void hash_pair<T, INDEX_SIZE>::it::begin(void) {
	if (instance) {
		instance->locate(pos_x, pos_y, pos_n, 0);
	}
}


template<typename T, unsigned int INDEX_SIZE>
T hash_pair<T, INDEX_SIZE>::it::operator*() {
	if (instance) {
		return instance->locate(pos_x, pos_y, pos_n, 0);
	}
	return (T)0;
}


template<typename T, unsigned int INDEX_SIZE>
bool hash_pair<T, INDEX_SIZE>::it::operator==(const typename hash_pair<T, INDEX_SIZE>::it& rv) {
	/* 判断是否等于end对象 */
	if (rv.instance == NULL) {
		if (rv.pos_x == this->pos_x) {
			return true;
		}
		return false;
	}
	
	/* 正常对象比较 */
	if ((rv.instance == this->instance) && (rv.pos_x == this->pos_x) 
		&& (rv.pos_y == this->pos_y) && (rv.pos_n == this->pos_n)) {
		return true;
	}
	return false;
}


template<typename T, unsigned int INDEX_SIZE>
bool hash_pair<T, INDEX_SIZE>::it::operator!=(const typename hash_pair<T, INDEX_SIZE>::it& rv) {
	/* 判断是否等于end对象 */
	if (rv.instance == NULL) {
		if (rv.pos_x != this->pos_x) {
			return true;
		}
		return false;
	}
	
	/* 正常对象比较 */
	if ((rv.instance != this->instance) || (rv.pos_x != this->pos_x) 
		|| (rv.pos_y != this->pos_y) || (rv.pos_n != this->pos_n)) {
		return true;
	}
	return false;
}


template<typename T, unsigned int INDEX_SIZE>
typename hash_pair<T, INDEX_SIZE>::it& hash_pair<T, INDEX_SIZE>::it::operator=
								(const typename hash_pair<T, INDEX_SIZE>::it& rv) {
	if (this != &rv) {
		this->instance = rv.instance;
		this->pos_x = rv.pos_x;
		this->pos_y = rv.pos_y;
		this->pos_n = rv.pos_n;
	}
	return *this;
}



template<typename T, unsigned int INDEX_SIZE>
typename hash_pair<T, INDEX_SIZE>::it hash_pair<T, INDEX_SIZE>::it::operator++(int) {
	it ret(this);
	if (instance) {
		instance->locate(pos_x, pos_y, pos_n, 1);
	}
	return ret;
}


template<typename T, unsigned int INDEX_SIZE>
typename hash_pair<T, INDEX_SIZE>::it& hash_pair<T, INDEX_SIZE>::it::operator++() {
	if (instance) {
		instance->locate(pos_x, pos_y, pos_n, 1);
	}
	return *this;
}

#endif /* __HASH_PAIR_H__ */
