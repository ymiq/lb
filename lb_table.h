

#ifndef _LB_TABLE_H__
#define _LB_TABLE_H__

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include "config.h"

using namespace std; 

typedef struct serv_info {
	int handle;
	unsigned int status;
}__attribute__((packed)) serv_info;

typedef struct lb_item{
	uint64_t hash;
	serv_info server;
}lb_item;

typedef struct lb_index {
	lb_item  items[CFG_ITEMS_SIZE];
	lb_index *next;
}lb_index;

class lb_table {
public:
	lb_table();
	~lb_table();

protected:
	/* When stoped, handle return -1 */
	int get_handle(uint64_t hash);
	int get_handle(uint64_t hash, int *stat);
	int set_handle(uint64_t hash, int handle);
	
	bool is_start(uint64_t hash);
	bool stop(uint64_t hash);
	bool start(uint64_t hash);
	
	bool is_stat(uint64_t hash);	
	bool stop_stat(uint64_t hash);
	bool start_stat(uint64_t hash);
	
private:
	lb_index *lb_idx;
	void *lb_idx_buf;
	
	lb_item *lb_get(uint64_t hash);
	lb_item *lb_update(lb_item *pitem);
	bool lb_delete(uint64_t hash);
};

#endif /* _LB_TABLE_H__ */
