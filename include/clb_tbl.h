

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

typedef struct lbsrv_info {
	int handle;
	unsigned int ip;
	unsigned short port;
	unsigned int lb_status;
	unsigned int stat_status;
}lbsrv_info;

typedef struct lb_item{
	unsigned long hash;
	lbsrv_info *server;
}lb_item;

typedef struct lb_index {
	lb_item  items[CFG_ITEMS_SIZE];
	lb_index *next;
}lb_index;

class clb_tbl {
public:
	static clb_tbl *get_inst() {
		static clb_tbl lb_singleton;
		return &lb_singleton;
	};

	/* When stoped, handle return -1 */
	int get_handle(unsigned long hash);
	int get_handle(unsigned long hash, 
			unsigned int *lb_status, unsigned int *stat_status);
	
	bool lb_delete(unsigned long hash);
	
	bool is_lb_start(unsigned long hash);
	int lb_stop(unsigned long hash);
	int lb_start(unsigned long hash);
	int lb_stop(unsigned long hash, int handle);
	int lb_start(unsigned long hash, int handle);
	int lb_info(unsigned long hash, lbsrv_info *info);
	
	bool is_stat_start(unsigned long hash);	
	int stat_start(unsigned long hash);
	int stat_stop(unsigned long hash);
	
protected:
	
private:
	lb_index *lb_idx;
	void *lb_idx_buf;
	rcu_obj<lbsrv_info> *info_list;
	
	clb_tbl();
	~clb_tbl();
	
	lbsrv_info *lbsrv_info_new(lbsrv_info *ref, int handle, int lb_status, int stat_status);
	lbsrv_info *lb_get(unsigned long hash);
	lbsrv_info *lb_update(unsigned long hash, int handle, int lb_status, int stat_status);
};

#endif /* _LB_TABLE_H__ */
