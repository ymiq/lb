#ifndef _STAT_MAN_H__
#define _STAT_MAN_H__

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <pthread.h>
 
#include <iostream>
#include <list>
#include <numeric>
#include <algorithm>

#include "config.h"
#include "stat_table.h"

using namespace std;

class stat_man {
public:
	static stat_man *get_inst() {
		static stat_man stat_singleton;
		return &stat_singleton;
	};
	int reg(stat_table *pstat);
	
	/* 统计操作基本方法 */
	int open(uint64_t hash);					/* 创建一个统计对象 */
	int close(uint64_t hash);					/* 销毁一个统计对象 */
	int start(uint64_t hash, uint32_t code);	/* 开启统计 */
	int stop(uint64_t hash, uint32_t code);		/* 暂停统计 */
	int clear(uint64_t hash, uint32_t code);	/* 清除统计 */
	int get(uint64_t hash, stat_info *pinfo);	/* 获取统计信息 */

protected:
	
private:
	#define CFG_THREAD_TABLE_SIZE	64

	typedef struct thread_table {
		stat_table *table[CFG_THREAD_TABLE_SIZE];
		struct thread_table *next;
	} thread_table;

	thread_table *stat_tbl;
	
	stat_man();
	~stat_man();
};

#endif /* _STAT_MAN_H__ */
