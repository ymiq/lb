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


// template<typename T, unsigned int INDEX_SIZE> class it;


template<typename T, unsigned int INDEX_SIZE>
class hash_tbl {
public:
	hash_tbl();
	~hash_tbl();

	int check(unsigned long hash);
	void remove(unsigned long hash);
	T *update(unsigned long hash, const T &obj);
	T *find(unsigned long hash);
	
	class it {
	public:
	    it() {};
	    ~it(){};
	    T *begin(void) {return NULL;}
	    T *next(void) {return NULL;}
	    unsigned long hash(void) { return 0;}
	private:
	};

	it &iterator(void);
		
protected:
	
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
	}hash_index;
	
	unsigned int mod_size;
	hash_index *hidx;
	void *hidx_buf;
	rcu_obj<T> *prcu;
	
};


template<typename T, unsigned int INDEX_SIZE>
hash_tbl<T, INDEX_SIZE>::hash_tbl()
{
	unsigned int size = INDEX_SIZE;
	
	/* 调整参数size为2^n */
	if (size >= 0x40000000) {
		size = 0x40000000;
	} else {
		int bit = 30;
		for (bit=30;  bit>=0; bit--) {
			if (size & (1UL << bit)) {
				break;
			}
		}
		size = 1UL << (bit + 1);
	}
	mod_size = size;
	
	/* 申请哈希表索引 */
	hidx_buf = (hash_index *) malloc(sizeof(hash_index) * size + CFG_CACHE_ALIGN);
	if (hidx_buf == NULL) {
		throw "No memory for hidx_buf";
	}
	memset(hidx_buf, 0x00, sizeof(hash_index) * size + CFG_CACHE_ALIGN);
	
	/* Cache Line对齐处理，加速搜索 */
	hidx = (hash_index*) (((size_t)hidx_buf) & ~(CFG_CACHE_ALIGN - 1));		
	
	/* RCU初始化 */
	prcu = new rcu_obj<T>();
	if (prcu == NULL) {
		throw "Can't create rcu_obj for hash_tbl";
	}
	prcu->set_type(OBJ_LIST_TYPE_CLASS);
}


template<typename T, unsigned int INDEX_SIZE>
hash_tbl<T, INDEX_SIZE>::~hash_tbl()
{
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
T *hash_tbl<T, INDEX_SIZE>::update(unsigned long hash, const T &obj)
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
typename hash_tbl<T, INDEX_SIZE>::it &hash_tbl<T, INDEX_SIZE>::iterator(void) {
	it *ret = new it;
	return *ret;
}


#endif /* __HASH_TABLE_H__ */
