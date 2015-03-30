/*
 *
 *	无锁动态键值对，用于存储键值对。
 * 	支持多个读者，多个写者。不需要使用RCU_MAN进行辅助更新处理。
 *  V : 要求值必须为指针，或者整型数据类型（但要求0值表示不合法数据）。
 *  KS：设置为MAX_HASHS/8比较合理。MAX_HASHS表示动态数组预计存储最多Hash值数量。
 *  KS：建议为2^n，如果不是，将会被强制调整。
 *  VS：设置为MAX_VALUES/8比较合理。MAX_VALUES表示动态数组预计存储最多Value值数量。
 *  VS：建议为2^n，如果不是，将会被强制调整。
 *
 *  内存使用约：(2 * KS + VS) × sizeof(unsigned long) × 32
 *
 *  注意：该模板未实现拷贝构造函数，创建的对象不能被复制，也不建议对该类实例化对象进行复制。
 *
 */

#ifndef __HASH_BIND_H__
#define __HASH_BIND_H__

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <evsock.h>
#include <evclnt.h>
#include <utils/hash_alg.h>
#include <hash_array.h>
#include <hash_pair.h>

using namespace std;

#define KEY_ARRAY	hash_array<KS>	

template<typename V, unsigned int KS, unsigned int VS>
class hash_bind {
public:
	hash_bind() {};
	~hash_bind() {};
	
	bool add(unsigned long key, V &value);
	void remove(unsigned long key);
	void remove(V &value);
	
	V get_val(unsigned long key);

protected:
	
private:
	hash_pair<V, KS> key_table;
	hash_pair<KEY_ARRAY *, VS> value_table;
};


template<typename V, unsigned int KS, unsigned int VS>
bool hash_bind<V, KS, VS>::add(unsigned long key, V &value) {
	bool ret = false;
	
	/* 添加到key_table */
	if (!key_table.add(key, value)) {
		return ret;
	}
	
	/* 添加到val_table */
	unsigned long vhash = pointer_hash(value);
	KEY_ARRAY *key_array = value_table.find(vhash);
	if (key_array) {
		ret = key_array->add(key);
	} else {
		key_array = new KEY_ARRAY();
		ret = key_array->add(key);
		if (ret) {
			ret = value_table.add(vhash, key_array);
		}
		if (!ret) {
			delete key_array;
		}
	}
	if (!ret) {
		key_table.remove(key);
	}
	return ret;
}


template<typename V, unsigned int KS, unsigned int VS>
void hash_bind<V, KS, VS>::remove(unsigned long key) {
	/* 删除key_table中相关内容 */
	V rv = key_table.find(key);
	if (rv != NULL) {
		key_table.remove(key);
		
		/* 删除val_table中相关内容 */
		unsigned long vhash = pointer_hash(rv);
		KEY_ARRAY *key_array = value_table.find(vhash);
		if (key_array) {
			key_array->remove(key);
		}
	}
}


template<typename V, unsigned int KS, unsigned int VS>
void hash_bind<V, KS, VS>::remove(V &value) {
	/* 获取KEY数组 */

	unsigned long vhash = pointer_hash(value);
	KEY_ARRAY *key_array = value_table.find(vhash);
	if (key_array == NULL) {
		return;
	}
	
	/* 删除val_table中内容 */
	value_table.remove(vhash);
	
	/* 循环删除所有KEY */
	typename KEY_ARRAY::it it;
	for (it=key_array->begin(); it!=key_array->end(); ++it) {
		unsigned long key = *it;
		key_array->remove(key);
		key_table.remove(key);
	}	
	delete key_array;
}


template<typename V, unsigned int KS, unsigned int VS>
V hash_bind<V, KS, VS>::get_val(unsigned long key) {
	V value = key_table.find(key);
	return value;
}


#endif /* __HASH_BIND_H__ */
