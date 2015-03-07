/*
 * 公司组管理HASH表
 * 一个二级HASH表
 */

#ifndef __CLB_GRP_H__
#define __CLB_GRP_H__

#include <cstdlib>
#include <cstddef>
#include <config.h>
#include <hash_tbl.h>
#include <clb_tbl.h>
#include <stat_tbl.h>

#define CLB_COMPANY_TBL		hash_tbl<clb_hash_info, 1024>

struct clb_hash_info
{
	unsigned long hash;
};

struct clb_grp_info {
	unsigned int group;
	int handle;
	unsigned int ip;
	unsigned short port;
	unsigned lb_status;
	unsigned stat_status;
	CLB_COMPANY_TBL *company_tbl;
};


class clb_grp {
public:
	static clb_grp *get_inst() {
		static clb_grp clb_grp_singleton;
		return &clb_grp_singleton;
	};
	
	void remove(unsigned int group);
	void remove(unsigned int group, unsigned long hash);
	clb_grp_info *find(unsigned int group);
	clb_grp_info *create(clb_grp_info &info, unsigned long hash);
	int get_handle(unsigned int group);
	
	int lb_start(unsigned int group);
	int lb_stop(unsigned int group);
	int stat_start(unsigned int group);
	int stat_stop(unsigned int group);

protected:
	
private:	
	hash_tbl<clb_grp_info, 64> table;
	clb_tbl *pclb;
	
	clb_grp();
	~clb_grp();
	unsigned long group_hash(unsigned int group);
	int open_sock(unsigned int master, unsigned short port);
};

#endif /* __CLB_GRP_H__ */
