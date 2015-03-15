#ifndef __STAT_MAN_H__
#define __STAT_MAN_H__

#include <cstdlib>
#include <cstddef>
#include <ctime>

#include <iostream>
#include <list>
#include <numeric>
#include <algorithm>

#include <pthread.h>
#include <config.h>
#include <stat/stat_tbl_base.h>

using namespace std;

class stat_man {
public:
	static stat_man *get_inst() {
		static stat_man stat_singleton;
		return &stat_singleton;
	};
	int reg(int type, stat_tbl_base *pstat);
	void unreg(int type, stat_tbl_base *pstat);
	
	/* 统计操作基本方法 */
	int start(int type, unsigned long hash, unsigned int code);	/* 开启统计 */
	int stop(int type, unsigned long hash);						/* 关闭统计 */
	int clear(int type, unsigned long hash, unsigned int code);	/* 清除统计 */
	int read(int type, unsigned long hash, 
					stat_info_base *pinfo, struct timeval *tm);	/* 获取统计信息 */

protected:
	
private:
	#define CFG_THREAD_TABLE_SIZE	64

	typedef struct thread_table {
		stat_tbl_base *ptables[CFG_THREAD_TABLE_SIZE];
		struct thread_table *pnext;
	} thread_table;

	typedef struct thread_table_list {
		int type;
		thread_table *ptable;
		thread_table_list *pnext;
	} thread_table_list;

	thread_table_list *plist_head;
	pthread_mutex_t table_mtx;		/* 线程锁，永远多个写者的竞争处理。 */
	
	stat_man();
	~stat_man();
	void lock(void);
	void unlock(void);
	
	int reg(thread_table *ptbl, stat_tbl_base *pstat);
	void unreg(thread_table *ptbl, stat_tbl_base *pstat);
	
	/* 统计操作基本方法 */
	int start(thread_table *ptbl, unsigned long hash, unsigned int code);	/* 开启统计 */
	int stop(thread_table *ptbl, unsigned long hash);						/* 关闭统计 */
	int clear(thread_table *ptbl, unsigned long hash, unsigned int code);	/* 清除统计 */
	int read(thread_table *ptbl, unsigned long hash, 
								stat_info_base *pinfo, struct timeval *tm);	/* 获取统计信息 */
};

#endif /* __STAT_MAN_H__ */
