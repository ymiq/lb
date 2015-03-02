#ifndef _STAT_MAN_H__
#define _STAT_MAN_H__

#include <cstdlib>
#include <cstddef>
#include <ctime>

#include <iostream>
#include <list>
#include <numeric>
#include <algorithm>

#include <pthread.h>
#include <config.h>
#include <stat_table.h>

using namespace std;

class stat_man {
public:
	static stat_man *get_inst() {
		static stat_man stat_singleton;
		return &stat_singleton;
	};
	int reg(stat_table *pstat);
	
	/* 统计操作基本方法 */
	int open(unsigned long int hash);					/* 创建一个统计对象 */
	int close(unsigned long int hash);					/* 销毁一个统计对象 */
	int start(unsigned long int hash, unsigned int code);	/* 开启统计 */
	int stop(unsigned long int hash, unsigned int code);	/* 暂停统计 */
	int clear(unsigned long int hash, unsigned int code);	/* 清除统计 */
	int read(unsigned long int hash, stat_info *pinfo, struct timeval *tm);	/* 获取统计信息 */

protected:
	
private:
	#define CFG_THREAD_TABLE_SIZE	64

	typedef struct thread_table {
		stat_table *table[CFG_THREAD_TABLE_SIZE];
		struct thread_table *next;
	} thread_table;

	thread_table *stat_tbl;
	pthread_mutex_t stat_mtx;
	
	stat_man();
	~stat_man();
	void lock(void);
	void unlock(void);
};

#endif /* _STAT_MAN_H__ */
