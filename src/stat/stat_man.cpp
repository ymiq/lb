#include <ctime>
#include <sys/time.h>
#include <log.h>
#include <stat/stat_man.h>

stat_man::stat_man() {
	plist_head = NULL;
	pthread_mutex_init(&table_mtx, NULL);
}


stat_man::~stat_man() {
	thread_table_list *plist = plist_head;
	thread_table_list *prev_list = plist;
	while (plist) {
		if (plist->type >= 0) {
			thread_table *prev_tbl;
			thread_table *ptbl = plist->ptable;
			while (ptbl) {
				prev_tbl = ptbl;
				ptbl = ptbl->pnext;
				delete prev_tbl;
			}
		}
		prev_list = plist;
		plist = plist->pnext;
		delete prev_list;
	}
}


void stat_man::lock(void) {
	pthread_mutex_lock(&table_mtx);
}


void stat_man::unlock(void) {
	pthread_mutex_unlock(&table_mtx);
}


int stat_man::reg(thread_table *ptable, stat_tbl_base *pstat) {
	int ret;
	thread_table *ptbl = ptable;
	thread_table *prev_tbl;
	
	/* 检查线程是否已经注册 */
	ret = 0;
	do
	{
		for (int i=0; i<CFG_THREAD_TABLE_SIZE; i++) {
			if (ptbl->ptables[i] == pstat) {
				return 0;
			}
		}
		prev_tbl = ptbl;
		ptbl = ptbl->pnext;
		ret += CFG_THREAD_TABLE_SIZE;
	}while (ptbl);
	
	/* 注册线程 */
	ret = 0;
	ptbl = ptable;
	do
	{
		for (int i=0; i<CFG_THREAD_TABLE_SIZE; i++) {
			if (ptbl->ptables[i] == 0) {
				ptbl->ptables[i] = pstat;
				return 1;
			}
		}
		prev_tbl = ptbl;
		ptbl = ptbl->pnext;
		ret += CFG_THREAD_TABLE_SIZE;
	}while (ptbl);
	
	/* 重新申请缓冲区 */
	thread_table *new_tbl = new thread_table();
	new_tbl->ptables[0] = pstat;
	
	/* 避免乱序造成问题 */
	wmb();
	prev_tbl->pnext = new_tbl;
	return 1;
}


int stat_man::reg(int type, stat_tbl_base *pstat) {
	/* 参数检查 */
	if ((type < 0) || !pstat) {
		return -1;
	}
	
	/* 检查当前类型是否已经注册 */
	lock();
	int ret = 0;
	thread_table_list *plist = plist_head;
	while (plist) {
		if (plist->type == type) {
			ret = reg(plist->ptable, pstat);
			unlock();
			return ret;
		}
		plist = plist->pnext;
	}
	
	/* 注册新的类型 */
	ret = 0;
	plist = plist_head;
	thread_table_list *prev_list = plist;
	while (plist) {
		if (plist->type == -1) {
			plist->ptable = new thread_table();
			wmb();
			plist->type = type;
			ret = reg(plist->ptable, pstat);
			unlock();
			return ret;
		}
		prev_list = plist;
		plist = plist->pnext;
	};
	
	/* 重新申请缓冲区 */
	thread_table_list *new_node = new thread_table_list();
	new_node->pnext = NULL;
	new_node->type = type;
	new_node->ptable = new thread_table();
	ret = reg(plist->ptable, pstat);
	
	/* 避免乱序造成问题 */
	wmb();
	prev_list->pnext = new_node;
	unlock();
	return ret;
}


void stat_man::unreg(thread_table *ptbl, stat_tbl_base *pstat) {
	/* 检查线程是否已经注册 */
	while (ptbl) {
		for (int i=0; i<CFG_THREAD_TABLE_SIZE; i++) {
			if (ptbl->ptables[i] == pstat) {
				ptbl->ptables[i] = NULL;
				return;
			}
		}
		ptbl = ptbl->pnext;
	}
}


void stat_man::unreg(int type, stat_tbl_base *pstat) {
	lock();
	thread_table_list *plist = plist_head;
	while (plist) {
		if (plist->type == type) {
			plist->type = -1;
			wmb();
			unreg(plist->ptable, pstat);
			unlock();
		}
		plist = plist->pnext;
	}
	unlock();
}


int stat_man::start(thread_table *ptable, unsigned long hash, unsigned int code) {
	int last = 0;
	thread_table *ptbl = ptable;
	stat_tbl_base *pstat;
	
	/* 遍历所有注册统计对象表 */
	do
	{
		for (int i=0; i<CFG_THREAD_TABLE_SIZE; i++) {
			pstat = ptbl->ptables[i];

			/* 搜索结束 */
			if (pstat == NULL) {
				return 0;
			}
			
			/* 打开指定线程下相应统计对象 */
			if (pstat->start(hash, code) < 0) {
				
				/* 回滚处理 */
				last += i;
				--last;
				ptbl = ptable;
	
				/* 遍历所有注册统计对象表, 进行销毁处理 */
				while(ptbl && (last >= 0))
				{
					for (int j=0; j<CFG_THREAD_TABLE_SIZE; j++) {
						stat_tbl_base *pstop = ptbl->ptables[j];
						if (pstop) {
							pstop->stop(hash);
						}
						if (--last == 0) {
							return -1;
						}
					}
					ptbl = ptbl->pnext;
				}
				
				/* 出错返回 */
				return -1;
			}
		}
		last += CFG_THREAD_TABLE_SIZE;
		ptbl = ptbl->pnext;
	}while (ptbl);
	
	return 0;
}


int stat_man::start(int type, unsigned long hash, unsigned int code) {
	thread_table_list *plist = plist_head;
	while (plist) {
		if (plist->type == type) {
			return start(plist->ptable, hash, code);
		}
		plist = plist->pnext;
	}
	return -1;
}


int stat_man::stop(thread_table *ptbl, unsigned long hash) {
	int ret = 0;
	stat_tbl_base *pstat;
	
	/* 遍历所有注册统计对象表 */
	do
	{
		for (int i=0; i<CFG_THREAD_TABLE_SIZE; i++) {
			pstat = ptbl->ptables[i];

			/* 搜索结束 */
			if (pstat == NULL) {
				return ret;
			}
			
			/* 关闭指定线程下相应统计对象 */
			if (pstat->stop(hash) < 0) {
				ret = -1;
			}
		}
		ptbl = ptbl->pnext;
	}while (ptbl);
	
	return ret;
}


int stat_man::stop(int type, unsigned long hash) {
	thread_table_list *plist = plist_head;
	while (plist) {
		if (plist->type == type) {
			return stop(plist->ptable, hash);
		}
		plist = plist->pnext;
	}
	return -1;
}


int stat_man::clear(thread_table *ptbl, unsigned long hash, unsigned int code) {
	int ret = 0;
	stat_tbl_base *pstat;
	
	/* 遍历所有注册统计对象表 */
	do
	{
		for (int i=0; i<CFG_THREAD_TABLE_SIZE; i++) {
			pstat = ptbl->ptables[i];

			/* 搜索结束 */
			if (pstat == NULL) {
				return ret;
			}
			
			/* 关闭指定线程下相应统计对象 */
			if (pstat->clear(hash, code) < 0) {
				ret = -1;
			}
		}
		ptbl = ptbl->pnext;
	}while (ptbl);
	
	return ret;
}


int stat_man::clear(int type, unsigned long hash, unsigned int code) {
	thread_table_list *plist = plist_head;
	while (plist) {
		if (plist->type == type) {
			return clear(plist->ptable, hash, code);
		}
		plist = plist->pnext;
	}
	return -1;
}


int stat_man::read(thread_table *ptbl, unsigned long hash, 
					stat_info_base *pinfo, struct timeval *tm) {
#if 1
	return -1;
#else						
	stat_obj_base obj;
	
	/* 遍历所有注册统计对象表 */
	do
	{
		for (int i=0; i<CFG_THREAD_TABLE_SIZE; i++) {
			/* 搜索结束 */
			if (ptbl->ptables[i] == NULL) {
				/* 获取统计时间 */
				if (gettimeofday(tm, NULL) < 0) {
					LOGE("gettimeofday failed");
				}
	
				return obj.read(pinfo);
			}
			
			/* 关闭指定线程下相应统计对象 */
			stat_obj_base *pobj = ptbl->ptables[i]->get(hash);
			if (pobj) {
				obj += pobj;
			}
		}
		ptbl = ptbl->pnext;
	}while (ptbl);
	
	/* 获取统计时间 */
	if (gettimeofday(tm, NULL) < 0) {
		LOGE("gettimeofday failed");
	}
	return obj.read(pinfo);
#endif
}


int stat_man::read(int type, unsigned long hash, 
					stat_info_base *pinfo, struct timeval *tm) {
	thread_table_list *plist = plist_head;
	while (plist) {
		if (plist->type == type) {
			return read(plist->ptable, hash, pinfo, tm);
		}
		plist = plist->pnext;
	}
	return -1;
}



