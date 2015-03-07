#include <ctime>
#include <sys/time.h>
#include <log.h>
#include <stat_man.h>

stat_man::stat_man() {
	stat_table = new thread_table();
	pthread_mutex_init(&stat_mtx, NULL);
}


stat_man::~stat_man() {
	/* 未考虑销毁 */
}


void stat_man::lock(void) {
	pthread_mutex_lock(&stat_mtx);
}


void stat_man::unlock(void) {
	pthread_mutex_unlock(&stat_mtx);
}


int stat_man::reg(stat_tbl *pstat) {
	int ret;
	thread_table *ptbl = stat_table;
	thread_table *prev_tbl;
	
	/* 检查线程是否已经注册 */
	lock();
	ret = 0;
	do
	{
		for (int i=0; i<CFG_THREAD_TABLE_SIZE; i++) {
			if (ptbl->table[i] == pstat) {
				unlock();
				return 0;
			}
		}
		prev_tbl = ptbl;
		ptbl = ptbl->next;
		ret += CFG_THREAD_TABLE_SIZE;
	}while (ptbl);
	
	/* 注册线程 */
	ret = 0;
	ptbl = stat_table;
	do
	{
		for (int i=0; i<CFG_THREAD_TABLE_SIZE; i++) {
			if (ptbl->table[i] == 0) {
				ptbl->table[i] = pstat;
				unlock();
				return 1;
			}
		}
		prev_tbl = ptbl;
		ptbl = ptbl->next;
		ret += CFG_THREAD_TABLE_SIZE;
	}while (ptbl);
	
	/* 重新申请缓冲区 */
	thread_table *new_tbl = (thread_table*) calloc(sizeof(thread_table), 1);
	if (new_tbl  == NULL) {
		unlock();
		throw "No memory to construct stat_man";
	}
	new_tbl->table[0] = pstat;
	
	/* 避免乱序造成问题 */
	wmb();
	prev_tbl->next = new_tbl;
	unlock();
	return 1;
}


int stat_man::start(unsigned long hash, unsigned int code) {
	int last = 0;
	thread_table *ptbl = stat_table;
	stat_tbl *pstat;
	
	/* 遍历所有注册统计对象表 */
	do
	{
		for (int i=0; i<CFG_THREAD_TABLE_SIZE; i++) {
			pstat = ptbl->table[i];

			/* 搜索结束 */
			if (pstat == NULL) {
				return 0;
			}
			
			/* 打开指定线程下相应统计对象 */
			if (pstat->start(hash, code) < 0) {
				
				/* 回滚处理 */
				last += i;
				--last;
				ptbl = stat_table;
	
				/* 遍历所有注册统计对象表, 进行销毁处理 */
				while(ptbl && (last >= 0))
				{
					for (int j=0; j<CFG_THREAD_TABLE_SIZE; j++) {
						stat_tbl *pstop = ptbl->table[j];
						if (pstop) {
							pstop->stop(hash);
						}
						if (--last == 0) {
							return -1;
						}
					}
					ptbl = ptbl->next;
				}
				
				/* 出错返回 */
				return -1;
			}
		}
		last += CFG_THREAD_TABLE_SIZE;
		ptbl = ptbl->next;
	}while (ptbl);
	
	return 0;
}


int stat_man::stop(unsigned long hash) {
	int ret = 0;
	thread_table *ptbl = stat_table;
	stat_tbl *pstat;
	
	/* 遍历所有注册统计对象表 */
	do
	{
		for (int i=0; i<CFG_THREAD_TABLE_SIZE; i++) {
			pstat = ptbl->table[i];

			/* 搜索结束 */
			if (pstat == NULL) {
				return ret;
			}
			
			/* 关闭指定线程下相应统计对象 */
			if (pstat->stop(hash) < 0) {
				ret = -1;
			}
		}
		ptbl = ptbl->next;
	}while (ptbl);
	
	return ret;
}


int stat_man::clear(unsigned long hash, unsigned int code) {
	int ret = 0;
	thread_table *ptbl = stat_table;
	stat_tbl *pstat;
	
	/* 遍历所有注册统计对象表 */
	do
	{
		for (int i=0; i<CFG_THREAD_TABLE_SIZE; i++) {
			pstat = ptbl->table[i];

			/* 搜索结束 */
			if (pstat == NULL) {
				return ret;
			}
			
			/* 关闭指定线程下相应统计对象 */
			if (pstat->clear(hash, code) < 0) {
				ret = -1;
			}
		}
		ptbl = ptbl->next;
	}while (ptbl);
	
	return ret;
}


int stat_man::read(unsigned long hash, stat_info *pinfo, struct timeval *tm) {
	thread_table *ptbl = stat_table;
	stat_obj obj;
	
	/* 遍历所有注册统计对象表 */
	do
	{
		for (int i=0; i<CFG_THREAD_TABLE_SIZE; i++) {
			/* 搜索结束 */
			if (ptbl->table[i] == NULL) {
				/* 获取统计时间 */
				if (gettimeofday(tm, NULL) < 0) {
					LOGE("gettimeofday failed");
				}
	
				return obj.read(pinfo);
			}
			
			/* 关闭指定线程下相应统计对象 */
			stat_obj *pobj = ptbl->table[i]->get(hash);
			if (pobj) {
				obj += pobj;
			}
		}
		ptbl = ptbl->next;
	}while (ptbl);
	
	/* 获取统计时间 */
	if (gettimeofday(tm, NULL) < 0) {
		LOGE("gettimeofday failed");
	}
	return obj.read(pinfo);
}



