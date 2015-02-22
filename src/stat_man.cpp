
#include "stat_man.h"

stat_man::stat_man() {
	stat_tbl = (thread_table*) calloc(sizeof(thread_table), 1);
	if (stat_tbl  == NULL) {
		throw "No memory to construct stat_man";
	}
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


int stat_man::reg(stat_table *pstat) {
	int ret;
	thread_table *ptbl = stat_tbl;
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
	ptbl = stat_tbl;
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


int stat_man::open(uint64_t hash) {
	thread_table *ptbl = stat_tbl;
	int last = 0;
	
	/* 遍历所有注册统计对象表 */
	do
	{
		for (int i=0; i<CFG_THREAD_TABLE_SIZE; i++) {
			/* 搜索结束 */
			if (ptbl->table[i] == NULL) {
				return 0;
			}
			
			/* 打开指定线程下相应统计对象 */
			if (ptbl->table[i]->open(hash) < 0) {
				
				/* 回滚处理 */
				last += i;
				--last;
				ptbl = stat_tbl;
	
				/* 遍历所有注册统计对象表, 进行销毁处理 */
				while(ptbl && (last >= 0))
				{
					for (int i=0; i<CFG_THREAD_TABLE_SIZE; i++) {
						ptbl->table[i]->close(hash);
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


int stat_man::close(uint64_t hash) {
	int ret = 0;
	thread_table *ptbl = stat_tbl;
	
	/* 遍历所有注册统计对象表 */
	do
	{
		for (int i=0; i<CFG_THREAD_TABLE_SIZE; i++) {
			/* 搜索结束 */
			if (ptbl->table[i] == NULL) {
				return ret;
			}
			
			/* 关闭指定线程下相应统计对象 */
			if (ptbl->table[i]->close(hash) < 0) {
				ret = -1;
			}
		}
		ptbl = ptbl->next;
	}while (ptbl);
	
	return ret;
}


int stat_man::start(uint64_t hash, uint32_t code) {
	int last = 0;
	thread_table *ptbl = stat_tbl;
	
	/* 遍历所有注册统计对象表 */
	do
	{
		for (int i=0; i<CFG_THREAD_TABLE_SIZE; i++) {
			/* 搜索结束 */
			if (ptbl->table[i] == NULL) {
				return 0;
			}
			
			/* 打开指定线程下相应统计对象 */
			if (ptbl->table[i]->start(hash, code) < 0) {
				
				/* 回滚处理 */
				last += i;
				--last;
				ptbl = stat_tbl;
	
				/* 遍历所有注册统计对象表, 进行销毁处理 */
				while(ptbl && (last >= 0))
				{
					for (int i=0; i<CFG_THREAD_TABLE_SIZE; i++) {
						ptbl->table[i]->stop(hash, code);
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


int stat_man::stop(uint64_t hash, uint32_t code) {
	int ret = 0;
	thread_table *ptbl = stat_tbl;
	
	/* 遍历所有注册统计对象表 */
	do
	{
		for (int i=0; i<CFG_THREAD_TABLE_SIZE; i++) {
			/* 搜索结束 */
			if (ptbl->table[i] == NULL) {
				return ret;
			}
			
			/* 关闭指定线程下相应统计对象 */
			if (ptbl->table[i]->stop(hash, code) < 0) {
				ret = -1;
			}
		}
		ptbl = ptbl->next;
	}while (ptbl);
	
	return ret;
}


int stat_man::clear(uint64_t hash, uint32_t code) {
	int ret = 0;
	thread_table *ptbl = stat_tbl;
	
	/* 遍历所有注册统计对象表 */
	do
	{
		for (int i=0; i<CFG_THREAD_TABLE_SIZE; i++) {
			/* 搜索结束 */
			if (ptbl->table[i] == NULL) {
				return ret;
			}
			
			/* 关闭指定线程下相应统计对象 */
			if (ptbl->table[i]->clear(hash, code) < 0) {
				ret = -1;
			}
		}
		ptbl = ptbl->next;
	}while (ptbl);
	
	return ret;
}


int stat_man::get(uint64_t hash, stat_info *pinfo) {
	int ret = 0;
	thread_table *ptbl = stat_tbl;
	stat_obj obj;
	
	/* 遍历所有注册统计对象表 */
	do
	{
		for (int i=0; i<CFG_THREAD_TABLE_SIZE; i++) {
			/* 搜索结束 */
			if (ptbl->table[i] == NULL) {
				return obj.get(pinfo);
			}
			
			/* 关闭指定线程下相应统计对象 */
			stat_obj *pobj = ptbl->table[i]->get(hash);
			if (pobj) {
				obj += pobj;
			}
		}
		ptbl = ptbl->next;
	}while (ptbl);
	
	return obj.get(pinfo);
}



