#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <exception>
#include <rcu_obj.h>
#include <rcu_man.h>
#include <stat/stat_man.h>
#include <stat/clb_stat.h>


using namespace std; 

clb_stat::clb_stat() {
	flags = 0;
	clear_flags = 0;
}


clb_stat::~clb_stat() {
}


int clb_stat::start(unsigned int code) {
	code &= CFG_STAT_MASK;
	flags |= code;
	return 0;
}


int clb_stat::stop(unsigned int code) {
	code &= CFG_STAT_MASK;
	flags &= ~code;
	return 0;
}


int clb_stat::clear(unsigned int code, int record) {
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


int clb_stat::clear(unsigned int code) {
	return clear(code, 1);
}


int clb_stat::read(stat_info_base *pinfo) {
	stat_info *pout = dynamic_cast<stat_info*>(pinfo);
	pout->total += this->info.total;
	pout->errors += this->info.errors;	
	pout->drops += this->info.drops;
	pout->texts += this->info.texts;	
	return 0;
}


int clb_stat::stat(unsigned int code) {
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
