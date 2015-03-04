#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <exception>
#include <log.h>
#include <clb_grp.h>
#include <rcu_man.h>


unsigned long clb_grp::group_hash(unsigned int group) {
	return (unsigned long)group;
}


void clb_grp::remove(unsigned int group, unsigned long hash) {
	unsigned long ghash = group_hash(group);
	GRP_TBL *grp_tbl = table.find(ghash);
	if (grp_tbl) {
		grp_tbl->remove(hash);
	}
}


clb_info *clb_grp::find(unsigned int group, unsigned long hash) {
	unsigned long ghash = group_hash(group);
	GRP_TBL *grp_tbl = table.find(ghash);
	if (!grp_tbl) {
		return NULL;
	} 
	return grp_tbl->find(hash);
}


clb_info *clb_grp::update(unsigned int group, unsigned long hash, clb_info &info) {
	unsigned long ghash = group_hash(group);
	GRP_TBL *grp_tbl = table.find(ghash);
	if (!grp_tbl) {
		return NULL;
	} 
	return grp_tbl->update(hash, info);
}


int clb_grp::lb_start(unsigned int group) {
	unsigned long ghash = group_hash(group);
	GRP_TBL *grp_tbl = table.find(ghash);
	
	if (!grp_tbl) {
		return -1;
	}

#if 0	
	GRP_TBL::it it = grp_tbl->iterator();
	clb_info *info = it.begin();
	while (info) {
		unsigned long hash = it.hash();
		grp_tbl->update(hash, *info);
		info = it.next();
	}
#else
	GRP_TBL::it it;
	for (it=grp_tbl->begin(); it!=grp_tbl->end; it++) {
		clb_info *info = *it();
		grp_tbl->update(hash, *info);
	}
	
#endif		
	return 0;
}
