

#ifndef __CLB_GRP_H__
#define __CLB_GRP_H__

#include <cstdlib>
#include <cstddef>
#include <config.h>
#include <hash_tbl.h>


class clb_info
{
public:
	clb_info() {};
	~clb_info() {};
	
	unsigned int ip;
	unsigned short port;
	int handle;
	bool lb_enable;
	bool stat_enable;
	
protected:
	
private:	
};


#define GRP_TBL		hash_tbl<clb_info, 1024>		

class clb_grp {
public:
	static clb_grp *get_inst() {
		static clb_grp clb_grp_singleton;
		return &clb_grp_singleton;
	};
	
	void remove(unsigned int group, unsigned long hash);
	clb_info *find(unsigned int group, unsigned long hash);
	clb_info *update(unsigned int group, unsigned long hash, clb_info &info);
	
	int lb_start(unsigned int group);

protected:
	
private:	
	hash_tbl<GRP_TBL, 64> table;
	
	clb_grp() {};
	~clb_grp() {};
	unsigned long group_hash(unsigned int group);
};

#endif /* __CLB_GRP_H__ */
