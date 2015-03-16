#ifndef __HASH_BIND_H__
#define __HASH_BIND_H__

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <evsock.h>
#include <evclnt.h>
#include <hash_array.h>

using namespace std;

#define KEY_ARRAY	hash_array<HASH, KS>	

template<typename K, typename V, unsigned int KS, unsigned int VS>
class hash_bind {
public:
	hash_bind() {};
	~hash_bind() {};
	
	int add(K key, V &value);
	int remove(K key);
	int remove(V &value);
	int update(K key, V &value);
	
	V get_val(K key);

protected:
	
private:
	hash_pair<HASH, KS> key_table;
	hash_pair<KEY_ARRAY *, VS> value_table;
};


template<typename K, typename V, unsigned int KS, unsigned int VS>
bool hash_bind<K, V, KS, VS>::add(K key, V &val) {
	/* 添加到key_table */
	V *rv = key_table.update(key, val);
	if (rv == NULL) {
		return false;
	}
	
	/* 添加到val_table */
	KEY_ARRAY *key_array = value_table.get(val);
	if (!key_array) {
		key_table.remove(key);
		return false;
	}
	bool ret = key_array.add(key);
	if (!ret) {
		key_table.remove(key);
	}
	return ret;
}


template<typename K, typename V, unsigned int KS, unsigned int VS>
bool hash_bind<K, V, KS, VS>::remove(K key) {
	/* 添加到key_table */
	V *rv = key_table.update(key, val);
	if (rv == NULL) {
		return false;
	}
	
	/* 添加到val_table */
	KEY_ARRAY *key_array = value_table.get(val);
	if (!key_array) {
		key_table.remove(key);
		return false;
	}
	bool ret = key_array.add(key);
	if (!ret) {
		key_table.remove(key);
	}
	return ret;
}

#endif /* __HASH_BIND_H__ */
