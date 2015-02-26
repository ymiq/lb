

#ifndef _LB_TABLE_H__
#define _LB_TABLE_H__

#include <cstdlib>
#include <cstddef>
#include <config.h>
#include <rcu_obj.h>

using namespace std; 

#define CFG_SERVER_LB_STOP			0
#define CFG_SERVER_LB_START			1
#define CFG_SERVER_STAT_STOP		0
#define CFG_SERVER_STAT_START		1

typedef struct server_info {
	int handle;
	bool lb_status;
	bool stat_status;
}server_info;

typedef struct lb_item{
	unsigned long int hash;
	server_info *server;
}lb_item;

typedef struct lb_index {
	lb_item  items[CFG_ITEMS_SIZE];
	lb_index *next;
}lb_index;

class lb_table {
public:
	static lb_table *get_inst() {
		static lb_table lb_singleton;
		return &lb_singleton;
	};

	/* When stoped, handle return -1 */
	int get_handle(unsigned long int hash);
	int get_handle(unsigned long int hash, bool *lb_status);
	
	bool lb_delete(unsigned long int hash);
	
	bool is_lb_start(unsigned long int hash);
	int lb_stop(unsigned long int hash);
	int lb_start(unsigned long int hash);
	int lb_stop(unsigned long int hash, int handle);
	int lb_start(unsigned long int hash, int handle);
	
	bool is_stat_start(unsigned long int hash);	
	int stat_start(unsigned long int hash);
	int stat_stop(unsigned long int hash);
	
protected:
	
private:
	lb_index *lb_idx;
	void *lb_idx_buf;
	rcu_obj<server_info> *info_list;
	
	lb_table();
	~lb_table();
	
	server_info *server_info_new(server_info *ref, int handle, int lb_status, int stat_status);
	server_info *lb_get(unsigned long int hash);
	server_info *lb_update(unsigned long int hash, int handle, int lb_status, int stat_status);
};

#endif /* _LB_TABLE_H__ */
