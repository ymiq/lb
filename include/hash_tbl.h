/*
 *	无锁HASH表，64位HASH值
 * 	支持多个读者，但支持一个写者!	     
 */

#ifndef __HASH_TABLE_H__
#define __HASH_TABLE_H__

#include <cstdlib>
#include <cstddef>
#include <config.h>
#include <vector>
#include <rcu_obj.h>

using namespace std;


template<typename T, unsigned int INDEX_SIZE>
class hash_tbl {

public:
	hash_tbl(const hash_tbl &tbl);
	hash_tbl();
	~hash_tbl();

	int check(unsigned long hash);
	void remove(unsigned long hash);
	T *update(unsigned long hash, T &obj);
	T *get(unsigned long hash);
	T *find(unsigned long hash);
	
	class it {
	public:
	    it(hash_tbl *instance = NULL, int end_x=0) 
	    			:instance(instance), pos_x(end_x), pos_y(0), pos_n(0) {}
	    ~it(){};

		T *operator*();
		it operator++(int);
		it &operator++();
		it &operator=(const it &rv);
		bool operator!=(const it &rv);
		bool operator==(const it &rv);
		
	protected:
	    void begin(void);
	    void next(void);
	
	private:
		hash_tbl *instance;
		int pos_x;
		int pos_y;
		int pos_n;
	};

	it begin(void);
	it &end(void);
		
protected:
	T *locate(int &pos_x, int &pos_y, int &pos_n, int inc);
	
private:
	/* 定义为2^n - 1 */
	#define CFG_HASH_ITEM_SIZE		7

	typedef struct hash_item{
		unsigned long hash;
		T *obj;
	}hash_item;
	
	typedef struct hash_index {
		hash_item  items[CFG_HASH_ITEM_SIZE];
		hash_index *next;
		void *reserve;
	}hash_index;
	
	unsigned int mod_size;
	hash_index *hidx;
	rcu_obj<T> *prcu;
	it *end_it;
	
};


template<typename T, unsigned int INDEX_SIZE>
hash_tbl<T, INDEX_SIZE>::hash_tbl()
{
	unsigned int size = INDEX_SIZE;
	
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
	
	/* RCU初始化 */
	prcu = new rcu_obj<T>();
	prcu->set_type(OBJ_LIST_TYPE_CLASS);
	
	end_it = new it(NULL, mod_size);
}


template<typename T, unsigned int INDEX_SIZE>
hash_tbl<T, INDEX_SIZE>::hash_tbl(const hash_tbl<T, INDEX_SIZE> &tbl) {
	throw "copy construct for hash_tbl is disabled";
}


template<typename T, unsigned int INDEX_SIZE>
hash_tbl<T, INDEX_SIZE>::~hash_tbl() {
#if 0	
	/* 递归删除所有HASH条目 */
	for(int x=0; x<mod_size; x++) {
		hash_index *pindex = &hidx[x];
		do {
			for (int n=0; n<CFG_HASH_ITEM_SIZE; n++) {
				
				unsigned long hash = pindex->items[n].hash;
				
				if (!hash) {
					break;
				} else if (hash != -1UL) {
					
					/* 删除条目 */
					pindex->items[n].hash = -1UL;
					wmb();
					if (pindex->items[n].obj) {
						prcu->add(pindex->items[n].obj);
					}
					pindex->items[n].obj = NULL;
				}
			}
			pindex = pindex->next;
		} while (pindex);
	}
	
	/* 删除NULL对象和索引表，保留RCU */	
	delete end_it;
	delete hidx;
#endif
}


template<typename T, unsigned int INDEX_SIZE>
T *hash_tbl<T, INDEX_SIZE>::locate(int &pos_x, int &pos_y, int &pos_n, int inc)
{
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
		if (++n >= CFG_HASH_ITEM_SIZE) {
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
			for (int idn=n; idn<CFG_HASH_ITEM_SIZE; idn++) {
				
				hash = pindex->items[idn].hash;
				
				/* 当前INDEX搜索结束 */
				if (!hash) {
					goto next;
				} else if (hash != -1UL) {
					
					/* 定位到有效条目 */
					pos_x = x;
					pos_y = y;
					pos_n = idn;
					return pindex->items[idn].obj;
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
T *hash_tbl<T, INDEX_SIZE>::find(unsigned long hash)
{
	unsigned int index;
	unsigned long save_hash;
	hash_index *pindex;
	
	if (!hash || (hash == -1UL)) {
		return NULL;
	}
	
	index = (unsigned int)(hash % mod_size);
	pindex = &hidx[index];
	
	do {
		for (int i=0; i<CFG_HASH_ITEM_SIZE; i++) {
			save_hash = pindex->items[i].hash;
			
			/* 搜索结束 */
			if (!save_hash) {
				return NULL;
			}
			
			/* 获得匹配 */
			if (save_hash == hash) {
				rmb();
				return pindex->items[i].obj;
			}
		}
		pindex = pindex->next;
	} while (pindex);
	
	return NULL;
}


template<typename T, unsigned int INDEX_SIZE>
T *hash_tbl<T, INDEX_SIZE>::update(unsigned long hash, T &obj)
{
	unsigned int index;
	unsigned long save_hash;
	hash_index *pindex, *prev;
	
	if (!hash || (hash == -1UL)) {
		return NULL;
	}
	
	T *pobj = new T(obj);
	index = (unsigned int)(hash % mod_size);
	prev = pindex = &hidx[index];
	
	/* 第一轮搜索，匹配是否存在该条目 */
	do {
		for (int i=0; i<CFG_HASH_ITEM_SIZE; i++) {
			save_hash = pindex->items[i].hash;
			
			/* 搜索结束 */
			if (!save_hash) {
				goto phase2;	
			}
			
			/* 获得匹配 */
			if (save_hash == hash) {
				wmb();
				pindex->items[i].obj = pobj;
				return pobj;
			}
		}
		pindex = pindex->next;
	} while (pindex);
	
	/* 第二轮搜索，插入/更新条目 */
phase2:
	pindex = prev;
	do {
		for (int i=0; i<CFG_HASH_ITEM_SIZE; i++) {
			save_hash = pindex->items[i].hash;

			/* 搜索结束，创建新条目 */
			if (!save_hash || (save_hash == -1UL)) {				

				/* 更新条目信息 */
				pindex->items[i].obj = pobj;

				/* 增加内存屏障，确保写入先后顺序 */
				wmb();
				pindex->items[i].hash = hash;
				return pobj;
			}
		}
		prev = pindex;
		pindex = pindex->next;
	} while (pindex);
	
	/* 申请新的索引项, 此处未再考虑Cache对齐；因为正常情况下概率较低 */
	pindex = new hash_index();
	
	/* 更新条目信息 */
	pindex->items[0].obj = pobj;
	pindex->items[0].hash = hash;

	/* 增加内存屏障，确保写入先后顺序 */
	wmb();
	prev->next = pindex;	
	return pobj;
}


template<typename T, unsigned int INDEX_SIZE>
T *hash_tbl<T, INDEX_SIZE>::get(unsigned long hash) {
	unsigned int index;
	unsigned long save_hash;
	hash_index *pindex, *prev;
	
	if (!hash || (hash == -1UL)) {
		return NULL;
	}
	
	index = (unsigned int)(hash % mod_size);
	prev = pindex = &hidx[index];
	
	/* 第一轮搜索，匹配是否存在该条目 */
	do {
		for (int i=0; i<CFG_HASH_ITEM_SIZE; i++) {
			save_hash = pindex->items[i].hash;
			
			/* 搜索结束 */
			if (!save_hash) {
				goto phase2;	
			}
			
			/* 获得匹配 */
			if (save_hash == hash) {
				rmb();
				return pindex->items[i].obj;
			}
		}
		pindex = pindex->next;
	} while (pindex);
	
	/* 第二轮搜索，插入/更新条目 */
phase2:
	T *pobj = new T();
	pindex = prev;
	do {
		for (int i=0; i<CFG_HASH_ITEM_SIZE; i++) {
			save_hash = pindex->items[i].hash;

			/* 搜索结束，创建新条目 */
			if (!save_hash || (save_hash == -1UL)) {				

				/* 更新条目信息 */
				pindex->items[i].obj = pobj;

				/* 增加内存屏障，确保写入先后顺序 */
				wmb();
				pindex->items[i].hash = hash;
				return pobj;
			}
		}
		prev = pindex;
		pindex = pindex->next;
	} while (pindex);
	
	/* 申请新的索引项, 此处未再考虑Cache对齐；因为正常情况下概率较低 */
	pindex = new hash_index();
	
	/* 更新条目信息 */
	pindex->items[0].obj = pobj;
	pindex->items[0].hash = hash;

	/* 增加内存屏障，确保写入先后顺序 */
	wmb();
	prev->next = pindex;	
	return pobj;
}



template<typename T, unsigned int INDEX_SIZE>
void hash_tbl<T, INDEX_SIZE>::remove(unsigned long hash)
{
	unsigned int index;
	unsigned long save_hash;
	hash_index *pindex;
	
	if (!hash || (hash == -1UL)) {
		return;
	}
	
	index = (unsigned int)(hash % mod_size);
	pindex = &hidx[index];
	
	do {
		for (int i=0; i<CFG_HASH_ITEM_SIZE; i++) {
			save_hash = pindex->items[i].hash;
			
			/* 搜索结束 */
			if (!save_hash) {
				return;
			}
			
			/* 获得匹配 */
			if (save_hash == hash) {
				
				/* 增加删除标记 */
				pindex->items[i].hash = -1UL;
				wmb();
				if (pindex->items[i].obj) {
					prcu->add(pindex->items[i].obj);
				}
				pindex->items[i].obj = NULL;
				return;
			}
		}
		pindex = pindex->next;
	} while (pindex);
	
	return;
}


template<typename T, unsigned int INDEX_SIZE>
typename hash_tbl<T, INDEX_SIZE>::it hash_tbl<T, INDEX_SIZE>::begin(void) {
	it ret(this);
	return ret;
}


template<typename T, unsigned int INDEX_SIZE>
typename hash_tbl<T, INDEX_SIZE>::it &hash_tbl<T, INDEX_SIZE>::end(void) {
	return *end_it;
}


template<typename T, unsigned int INDEX_SIZE>
T *hash_tbl<T, INDEX_SIZE>::it::operator*() {
	if (instance) {
		return instance->locate(pos_x, pos_y, pos_n, 0);
	}
	return NULL;
}


template<typename T, unsigned int INDEX_SIZE>
bool hash_tbl<T, INDEX_SIZE>::it::operator==(const typename hash_tbl<T, INDEX_SIZE>::it& rv) {
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
bool hash_tbl<T, INDEX_SIZE>::it::operator!=(const typename hash_tbl<T, INDEX_SIZE>::it& rv) {
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
typename hash_tbl<T, INDEX_SIZE>::it& hash_tbl<T, INDEX_SIZE>::it::operator=
								(const typename hash_tbl<T, INDEX_SIZE>::it& rv) {
	if (this != &rv) {
		this->instance = rv.instance;
		this->pos_x = rv.pos_x;
		this->pos_y = rv.pos_y;
		this->pos_n = rv.pos_n;
	}
	return *this;
}



template<typename T, unsigned int INDEX_SIZE>
typename hash_tbl<T, INDEX_SIZE>::it hash_tbl<T, INDEX_SIZE>::it::operator++(int) {
	it ret(this);
	if (instance) {
		instance->locate(pos_x, pos_y, pos_n, 1);
	}
	return ret;
}


template<typename T, unsigned int INDEX_SIZE>
typename hash_tbl<T, INDEX_SIZE>::it& hash_tbl<T, INDEX_SIZE>::it::operator++() {
	if (instance) {
		instance->locate(pos_x, pos_y, pos_n, 1);
	}
	return *this;
}

#endif /* __HASH_TABLE_H__ */
