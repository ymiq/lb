#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <exception>
#include <log.h>
#include <clb_tbl.h>
#include <rcu_man.h>

clb_tbl::clb_tbl() {
}


clb_tbl::~clb_tbl() {
}


int clb_tbl::create(lbsrv_info &info) {
	unsigned long hash = info.hash;
	lbsrv_info *ret = table.update(hash, info);
	if (!ret) {
		return -1;
	}
	return 0;
}


void clb_tbl::remove(unsigned long hash) {
	table.remove(hash);
}


int clb_tbl::lb_handle(unsigned long hash) {
	lbsrv_info *pserver = table.find(hash);
	if (pserver == NULL) {
		return -1;
	}
	return pserver->handle;
}


int clb_tbl::lb_handle(unsigned long hash, unsigned int &lb_status, unsigned int &stat_status) {
	lbsrv_info *pserver = table.find(hash);
	if (pserver == NULL) {
		return -1;
	}
	lb_status = pserver->lb_status;
	stat_status = pserver->stat_status;
	return pserver->handle;
}


int clb_tbl::lb_info(unsigned long hash, lbsrv_info *info) {
	lbsrv_info *pserver = table.find(hash);
	if (pserver == NULL) {
		return -1;
	}
	if (info) {
		*info = *pserver;
	}
	return 0;
}


bool clb_tbl::is_lb_start(unsigned long hash) {
	lbsrv_info *pserver = table.find(hash);
	if (pserver == NULL) {
		return false;
	}
	return pserver->lb_status ? true : false;
}


int clb_tbl::lb_start(unsigned long hash) {
	lbsrv_info *pserver = table.find(hash);
	if (pserver == NULL) {
		return -1;
	}
	int ret = pserver->lb_status;
	pserver->lb_status = 1;
	return ret;
}


int clb_tbl::lb_stop(unsigned long hash) {
	lbsrv_info *pserver = table.find(hash);
	if (pserver == NULL) {
		return -1;
	}
	int ret = pserver->lb_status;
	pserver->lb_status = 0;
	return ret;
}


bool clb_tbl::is_stat_start(unsigned long hash) {
	lbsrv_info *pserver = table.find(hash);
	if (pserver == NULL) {
		return -1;
	}
	return pserver->stat_status ? true: false;
}


int clb_tbl::stat_start(unsigned long hash) {
	lbsrv_info *pserver = table.find(hash);
	if (pserver == NULL) {
		return -1;
	}
	int ret = pserver->stat_status;
	pserver->stat_status = 1;
	return ret;
}


int clb_tbl::stat_stop(unsigned long hash) {
	lbsrv_info *pserver = table.find(hash);
	if (pserver == NULL) {
		return -1;
	}
	int ret = pserver->stat_status;
	pserver->stat_status = 0;
	return ret;
}


