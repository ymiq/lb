﻿#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <exception>
#include <log.h>
#include <clb_grp.h>
#include <clb_tbl.h>
#include <stat_tbl.h>

clb_grp::clb_grp() {
	pclb = clb_tbl::get_inst();
}


clb_grp::~clb_grp() {
	hash_tbl<clb_grp_info, 64>::it it;
	for (it=table.begin(); it!=table.end(); ++it) {
		clb_grp_info *grp_info = *it;
		if (grp_info) {
			if (grp_info->company_tbl) {
				delete grp_info->company_tbl;
			}
		}
	}
}


unsigned long clb_grp::group_hash(unsigned int group) {
	return (unsigned long)(group + 1);
}


void clb_grp::remove(unsigned int group) {
	table.remove(group_hash(group));
}


void clb_grp::remove(unsigned int group, unsigned long hash) {
	clb_grp_info *grp_info = table.find(group_hash(group));
	if (grp_info && grp_info->company_tbl) {
		grp_info->company_tbl->remove(hash);
	}
}


clb_grp_info *clb_grp::find(unsigned int group) {
	return table.find(group_hash(group));
}


clb_grp_info *clb_grp::create(clb_grp_info &info, unsigned long hash) {
	unsigned int group = info.group;
	unsigned long ghash;
	clb_grp_info *grp_info;
	
	ghash = group_hash(group);
	grp_info = table.find(ghash);
	if (grp_info == NULL) {
		info.company_tbl = new CLB_COMPANY_TBL();
		grp_info = table.update(ghash, info);
		if (grp_info == NULL) {
			return NULL;
		}
	}
	clb_hash_info hash_info;
	hash_info.hash = hash;
	if (grp_info->company_tbl->update(hash, hash_info) == NULL) {
		return NULL;
	}
	return grp_info;
}


int clb_grp::lb_start(unsigned int group) {
	clb_grp_info *grp_info = table.find(group_hash(group));
	if (!grp_info) {
		return -1;
	}

	CLB_COMPANY_TBL::it it;
	for (it=grp_info->company_tbl->begin(); it!=grp_info->company_tbl->end(); ++it) {
		clb_hash_info *hash_info = *it;
		if (hash_info && hash_info->hash) {
			pclb->lb_start(hash_info->hash);
		}
	}
	
	return 0;
}


int clb_grp::lb_stop(unsigned int group) {
	clb_grp_info *grp_info = table.find(group_hash(group));
	if (!grp_info) {
		return -1;
	}

	CLB_COMPANY_TBL::it it;
	for (it=grp_info->company_tbl->begin(); it!=grp_info->company_tbl->end(); ++it) {
		clb_hash_info *hash_info = *it;
		if (hash_info && hash_info->hash) {
			pclb->lb_stop(hash_info->hash);
		}
	}
	return 0;
}



int clb_grp::stat_start(unsigned int group) {
	clb_grp_info *grp_info = table.find(group_hash(group));
	if (!grp_info) {
		return -1;
	}

	CLB_COMPANY_TBL::it it;
	for (it=grp_info->company_tbl->begin(); it!=grp_info->company_tbl->end(); ++it) {
		clb_hash_info *hash_info = *it;
		if (hash_info && hash_info->hash) {
			pclb->stat_start(hash_info->hash);
		}
	}
	
	return 0;
}


int clb_grp::stat_stop(unsigned int group) {
	clb_grp_info *grp_info = table.find(group_hash(group));
	if (!grp_info) {
		return -1;
	}

	CLB_COMPANY_TBL::it it;
	for (it=grp_info->company_tbl->begin(); it!=grp_info->company_tbl->end(); ++it) {
		clb_hash_info *hash_info = *it;
		if (hash_info && hash_info->hash) {
			pclb->stat_stop(hash_info->hash);
		}
	}
	
	return 0;
}
