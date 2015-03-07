#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <stat_obj.h>
#include <rcu_obj.h>
#include <rcu_man.h>
#include <stat_man.h>

#include <exception>

using namespace std; 

stat_obj::stat_obj() {
	flags = 0;
	clear_flags = 0;
	memset(&info, 0, sizeof(info));
}


stat_obj::~stat_obj() {
}


int stat_obj::start(unsigned int code) {
	code &= CFG_STAT_MASK;
	flags |= code;
	return 0;
}


int stat_obj::stop(unsigned int code) {
	code &= CFG_STAT_MASK;
	flags &= ~code;
	return 0;
}


int stat_obj::clear(unsigned int code, int record) {
	code &= CFG_STAT_MASK;
	
	/* 清除统计计数器 */
	if (code & CFG_STAT_TOTAL) {
		info.total = 0;
	}
	if (code & CFG_STAT_DROP) {
		info.drops = 0;
	}
	if (code & CFG_STAT_ERROR) {
		info.errors = 0;
	}
	if (code & CFG_STAT_TEXT) {
		info.texts = 0;
	}
	
	/* 设置重复清除标记，避免多线程冲突 */
	if (record) {
		__sync_fetch_and_or(&clear_flags, code);
	} else {
		clear_flags = 0;
	}
	return 0;
}


int stat_obj::clear(unsigned int code) {
	return clear(code, 1);
}


int stat_obj::read(stat_info *pinfo) {
	*pinfo = info;
	return 0;
}


int stat_obj::stat(void *packet, int packet_size) {
	/* 重复清除，减小冲突 */
	unsigned int cflgs = clear_flags;
	if (cflgs) {
		clear(cflgs, 0);
	}
	
	if (flags & CFG_STAT_TOTAL) {
		info.total++;
	}
	return 0;
}


int stat_obj::stat(unsigned int code) {
	code = code & flags;
	
	/* 重复清除，减小冲突 */
	if (clear_flags) {
		clear(clear_flags, 0);
		clear_flags = 0;
	}

	/* 增加统计计数器 */
	if (code & CFG_STAT_TOTAL) {
		info.total++;
	}
	if (code & CFG_STAT_DROP) {
		info.drops++;
	}
	if (code & CFG_STAT_ERROR) {
		info.errors++;
	}
	if (code & CFG_STAT_TEXT) {
		info.texts++;
	}
	
	return 0;
}


stat_obj& stat_obj::operator+=(const stat_obj *pobj) {	
	this->info.total += pobj->info.total;
	this->info.errors += pobj->info.errors;	
	return *this;
}


stat_obj& stat_obj::operator+=(const stat_obj &obj) {	
	this->info.total += obj.info.total;
	this->info.errors += obj.info.errors;	
	return *this;
}
