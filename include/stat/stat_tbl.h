

#ifndef __STAT_TABLE_H__
#define __STAT_TABLE_H__

#include <cstdlib>
#include <cstddef>
#include <config.h>
#include <hash_tbl.h>
#include <stat/stat_obj_base.h>
#include <stat/stat_tbl_base.h>
#include <stat/stat_man.h>

using namespace std; 

template<typename T, unsigned int INDEX_SIZE>
class stat_tbl : public stat_tbl_base {
public:
	stat_tbl(int type = 0);
	~stat_tbl();
	
	/* 统计处理函数 */
	int stat(unsigned long hash, unsigned int code);
	int stat(unsigned long hash, void *packet, int packet_size);

	int start(unsigned long hash, unsigned int code);		/* 开启统计 */
	int stop(unsigned long hash);							/* 暂停统计 */
	int clear(unsigned long hash, unsigned int code);		/* 清除统计 */
	int read(unsigned long hash, stat_info_base *pinfo);	/* 获取统计数据 */

protected:
	
private:
	int tbl_type;
	hash_tbl<T, INDEX_SIZE> table;
};

template<typename T, unsigned int INDEX_SIZE>
stat_tbl<T, INDEX_SIZE>::stat_tbl(int type) : tbl_type(type) {
	/* 注册到统计管理器 */
	stat_man *pstat_man = stat_man::get_inst();
	pstat_man->reg(type, this);
}


template<typename T, unsigned int INDEX_SIZE>
stat_tbl<T, INDEX_SIZE>::~stat_tbl() {
	/* 从统计管理器中取消注册 */
	stat_man *pstat_man = stat_man::get_inst();
	pstat_man->unreg(tbl_type, this);
}


template<typename T, unsigned int INDEX_SIZE>
int stat_tbl<T, INDEX_SIZE>::start(unsigned long hash, unsigned int code) {
	T *pobj = table.get(hash);
	if (!pobj) {
		return -1;
	}
	return pobj->start(code);
}


template<typename T, unsigned int INDEX_SIZE>
int stat_tbl<T, INDEX_SIZE>::stop(unsigned long hash) {
	table.remove(hash);
	return 0;
}


template<typename T, unsigned int INDEX_SIZE>
int stat_tbl<T, INDEX_SIZE>::clear(unsigned long hash, unsigned int code) {
	T *pobj = table.find(hash);
	if (!pobj) {
		return -1;
	}
	return pobj->clear(hash);
}


template<typename T, unsigned int INDEX_SIZE>
int stat_tbl<T, INDEX_SIZE>::read(unsigned long hash, stat_info_base *pinfo) {
	T *pobj = table.find(hash);
	if (!pobj) {
		return -1;
	}
	return pobj->read(pinfo);
}


template<typename T, unsigned int INDEX_SIZE>
int stat_tbl<T, INDEX_SIZE>::stat(unsigned long hash, unsigned int code) {
	T *pobj = table.find(hash);
	if (!pobj) {
		return -1;
	}
	return pobj->stat(code);
}

#endif /* __STAT_TABLE_H__ */
