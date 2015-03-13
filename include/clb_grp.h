/*
 * 公司组管理HASH表
 * 一个二级HASH表
 */

#ifndef __CLB_GRP_H__
#define __CLB_GRP_H__

#include <cstdlib>
#include <cstddef>
#include <config.h>
#include <hash_array.h>
#include <hash_tbl.h>
#include <clb_tbl.h>
#include <stat_tbl.h>
#include <evclnt.h>
#include <clb_clnt.h>

#define GROUP_HASH_ARRAY		hash_array<1024>

struct clb_grp_info {
	unsigned int group;
	evclnt<clb_clnt> *pclnt;
	struct in_addr ip;
	unsigned short port;
	unsigned lb_status;
	unsigned stat_status;
	GROUP_HASH_ARRAY *phashs;
};


class clb_grp {
public:
	static clb_grp *get_inst() {
		static clb_grp clb_grp_singleton;
		return &clb_grp_singleton;
	};
	
	void remove(unsigned int group);
	void remove(unsigned int group, unsigned long hash);
	clb_grp_info *move(unsigned int src_group, unsigned int dst_group);
	clb_grp_info *find(unsigned int group);
	clb_grp_info *create(clb_grp_info &info);
	clb_grp_info *create(clb_grp_info &info, unsigned long hash);
	evclnt<clb_clnt> *get_clnt(unsigned int group);
	
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
	evclnt<clb_clnt> *open_clnt(struct in_addr ip, unsigned short port);
};

#endif /* __CLB_GRP_H__ */
