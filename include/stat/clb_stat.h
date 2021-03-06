﻿

#ifndef __CLB_STAT_OBJ_H__
#define __CLB_STAT_OBJ_H__

#include <cstdlib>
#include <cstddef>
#include <config.h>
#include <stat/stat_obj_base.h>

using namespace std; 

#define CFG_STAT_TOTAL		(1UL<<0)		/* 统计总问题 */
#define CFG_STAT_DROP		(1UL<<1)		/* 统计丢弃的问题 */
#define CFG_STAT_ERROR		(1UL<<2)		/* 统计错误的数据包 */
#define CFG_STAT_TEXT		(1UL<<3)		/* 统计文本问题 */

#define CFG_STAT_MASK		(CFG_STAT_TOTAL | CFG_STAT_DROP | CFG_STAT_ERROR | CFG_STAT_TEXT)

class stat_info: public stat_info_base {
public:
	stat_info(){};
	~stat_info(){};

	unsigned long total;
	unsigned long errors;
	unsigned long drops;
	unsigned long texts;
};

/* 该对象在单线程中调用，不用考虑重入问题!!! */
class clb_stat : public stat_obj_base {
public:
	clb_stat();
	~clb_stat();
	
	/* 统计触发 */
	int stat(unsigned int code);

	/* 统计操作基本方法 */
	int start(unsigned int code);		/* 开启统计 */
	int stop(unsigned int code);		/* 清除统计 */
	int clear(unsigned int code);		/* 清除统计 */
	int read(stat_info_base *pinfo);	/* 获取统计信息 */
	
protected:
	
private:
	unsigned int flags;
	unsigned int clear_flags;
	stat_info info;

	int clear(unsigned int code, int record);
};

#endif /* __CLB_STAT_OBJ_H__ */
