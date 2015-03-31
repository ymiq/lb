

#ifndef __CLB_TABLE_H__
#define __CLB_TABLE_H__

#include <cstdlib>
#include <cstddef>
#include <config.h>
#include <hash_tbl.h>
#include <evclnt.h>
#include <clb/clb_clnt.h>

using namespace std; 

#define CFG_SERVER_LB_STOP			0
#define CFG_SERVER_LB_START			1
#define CFG_SERVER_STAT_STOP		0
#define CFG_SERVER_STAT_START		1

typedef struct lbsrv_info {
	unsigned long hash;
	unsigned int group;
	evclnt<clb_clnt> *pclnt;
	unsigned int lb_status;
	unsigned int stat_status;
}lbsrv_info;

class clb_tbl {
public:
	static clb_tbl *get_inst() {
		static clb_tbl lb_singleton;
		return &lb_singleton;
	};

	void remove(unsigned long hash);
	int create(lbsrv_info &info);
	
	unsigned int group_id(unsigned long hash);
	
	evclnt<clb_clnt> *get_clnt(unsigned long hash);
	evclnt<clb_clnt> *get_clnt(unsigned long hash, unsigned int &lb_status, unsigned int &stat_status);
	
	bool is_lb_start(unsigned long hash);
	int lb_stop(unsigned long hash);
	int lb_start(unsigned long hash);
	int lb_switch(unsigned long hash, unsigned int group, evclnt<clb_clnt> *pclnt);
	int lb_info(unsigned long hash, lbsrv_info *info);
	
	bool is_stat_start(unsigned long hash);	
	int stat_start(unsigned long hash);
	int stat_stop(unsigned long hash);
	
protected:
	
private:
	hash_tbl<lbsrv_info, CFG_ITEMS_SIZE> table;
	
	clb_tbl();
	~clb_tbl();
};

#endif /* __CLB_TABLE_H__ */
