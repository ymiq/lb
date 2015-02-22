

#ifndef _STAT_TABLE_H__
#define _STAT_TABLE_H__

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include "config.h"
#include "stat_obj.h"

using namespace std; 
template<typename T> class rcu_obj;


typedef struct stat_item{
	uint64_t hash;
	stat_obj *obj;
}stat_item;

typedef struct stat_index {
	stat_item  items[CFG_ITEMS_SIZE];
	stat_index *next;
}stat_index;

class stat_table {
public:
	stat_table();
	~stat_table();
	
	/* 统计处理函数 */
	int stat(uint64_t hash);
	int stat(uint64_t hash, void *packet, int packet_size);
	int error_stat(uint64_t hash);

	int open(uint64_t hash);		/* 创建一个统计对象 */
	int close(uint64_t hash);		/* 销毁一个统计对象 */
	
	int start(uint64_t hash, uint32_t code);	/* 开启统计 */
	int stop(uint64_t hash, uint32_t code);		/* 暂停统计 */
	int clear(uint64_t hash, uint32_t code);	/* 清除统计 */
	stat_obj *get(uint64_t hash);				/* 获取对象 */

protected:
	
private:
	stat_index *stat_idx;
	void *stat_idx_buf;
	rcu_obj<stat_obj> *obj_list;
	
	stat_obj *stat_get(uint64_t hash);
	stat_obj *stat_new(uint64_t hash);
	bool stat_delete(uint64_t hash);
};

#endif /* _STAT_TABLE_H__ */
