

#ifndef _STAT_TABLE_H__
#define _STAT_TABLE_H__

#include <cstdlib>
#include <cstddef>
#include <config.h>
#include <stat_obj.h>

using namespace std; 
template<typename T> class rcu_obj;


typedef struct stat_item{
	unsigned long int hash;
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
	int stat(unsigned long int hash, unsigned int code);
	int stat(unsigned long int hash, void *packet, int packet_size);

	int start(unsigned long int hash, unsigned int code);	/* 开启统计 */
	int stop(unsigned long int hash);						/* 暂停统计 */
	int clear(unsigned long int hash, unsigned int code);	/* 清除统计 */
	stat_obj *get(unsigned long int hash);					/* 获取对象 */

protected:
	
private:
	stat_index *stat_idx;
	void *stat_idx_buf;
	rcu_obj<stat_obj> *obj_list;
	
	stat_obj *stat_get(unsigned long int hash);
	stat_obj *stat_new(unsigned long int hash);
	bool stat_delete(unsigned long int hash);
};

#endif /* _STAT_TABLE_H__ */
