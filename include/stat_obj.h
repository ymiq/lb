﻿

#ifndef _STAT_OBJ_H__
#define _STAT_OBJ_H__

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include "config.h"

using namespace std; 

typedef struct stat_info {
	unsigned long total_packets;
	unsigned long error_packets;
}__attribute__((packed)) stat_info;

/* 该对象在单线程中调用，不用考虑重入问题!!! */
class stat_obj {
public:
	stat_obj();
	~stat_obj();
	
	/* 统计触发 */
	int stat(void *packet, int packet_size);

	/* 统计操作基本方法 */
	int start(uint32_t code);	/* 开启统计 */
	int stop(uint32_t code);	/* 清除统计 */
	int clear(uint32_t code);	/* 清除统计 */
	int get(stat_info *pinfo);	/* 获取统计信息 */
	
	/* 操作符重载 */
	stat_obj& operator+=(const stat_obj *pobj);
	
protected:
	
private:
	uint32_t flags;
	stat_info info;
};

#endif /* _STAT_OBJ_H__ */
