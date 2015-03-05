#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <log.h>
#include <hash_tbl.h>
#include <stat_obj.h>
#include <stat_tbl.h>
#include <stat_man.h>

using namespace std; 

stat_tbl::stat_tbl() {
	/* 注册到统计管理器 */
	stat_man *pstat_man = stat_man::get_inst();
	pstat_man->reg(this);
}


stat_tbl::~stat_tbl() {
}


int stat_tbl::start(unsigned long hash, unsigned int code) {
	stat_obj *pobj = table.get(hash);
	if (!pobj) {
		return -1;
	}
	return pobj->start(code);
}


int stat_tbl::stop(unsigned long hash) {
	table.remove(hash);
	return 0;
}


int stat_tbl::clear(unsigned long hash, unsigned int code) {
	stat_obj *pobj = table.find(hash);
	if (!pobj) {
		return -1;
	}
	return pobj->clear(hash);
}


stat_obj *stat_tbl::get(unsigned long hash) {
	return table.find(hash);
}


int stat_tbl::stat(unsigned long hash, void *packet, int packet_size) {
	stat_obj *pobj = table.find(hash);
	if (!pobj) {
		return -1;
	}
	return pobj->stat(packet, packet_size);
}


int stat_tbl::stat(unsigned long hash, unsigned int code) {
	stat_obj *pobj = table.find(hash);
	if (!pobj) {
		return -1;
	}
	return pobj->stat(code);
}

