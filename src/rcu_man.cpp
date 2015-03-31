
#include <cstring>
#include <utils/log.h>
#include <rcu_man.h>
#include <unistd.h>

rcu_man::rcu_man() {
	obj_tbl = new obj_table();
	tid_tbl = new thread_table();
}


rcu_man::~rcu_man() {
}

bool rcu_man::obj_reg(rcu_base *obj) {
	obj_table *ptbl = obj_tbl;
	obj_table *prev_tbl;
	
	/* 检查释放已经注册 */
	do
	{
		for (int i=0; i<CFG_OBJ_TABLE_SIZE; i++) {
			if (ptbl->table[i] == obj) {
				return true;
			}
		}
		prev_tbl = ptbl;
		ptbl = ptbl->next;
	}while (ptbl);
	
	/* 注册指针 */
	ptbl = obj_tbl;
	do
	{
		for (int i=0; i<CFG_OBJ_TABLE_SIZE; i++) {
			rcu_base *save_obj = ptbl->table[i];
			if (save_obj == NULL) {
				if (__sync_bool_compare_and_swap(&ptbl->table[i], NULL, obj)) {
					return true;
				}
				return obj_reg(obj);				
			}
		}
		prev_tbl = ptbl;
		ptbl = ptbl->next;
	}while (ptbl);
	
	/* 重新申请缓冲区 */
	obj_table *new_tbl = new obj_table();
	new_tbl->table[0] = obj;
	wmb();
	
	/* 增加obj_table */
	if (__sync_bool_compare_and_swap(&prev_tbl->next, NULL, new_tbl)) {
		return true;
	}
	delete new_tbl;
	
	return obj_reg(obj);;
}


int rcu_man::tid_reg(pthread_t tid) {
	int ret;
	thread_table *ptbl = tid_tbl;
	thread_table *prev_tbl;
	
	/* 检查线程是否已经注册 */
	ret = 0;
	do
	{
		for (int i=0; i<CFG_THREAD_TABLE_SIZE; i++) {
			if (ptbl->table[i] == tid) {
				return ret + i;
			}
		}
		prev_tbl = ptbl;
		ptbl = ptbl->next;
		ret += CFG_THREAD_TABLE_SIZE;
	}while (ptbl);
	
	/* 注册线程 */
	ret = 0;
	ptbl = tid_tbl;
	do
	{
		for (int i=0; i<CFG_THREAD_TABLE_SIZE; i++) {
			pthread_t save_tid = ptbl->table[i];
			if (save_tid == 0) {
				if (__sync_bool_compare_and_swap(&ptbl->table[i], 0, tid)) {
					return ret + i;
				}
				return tid_reg(tid);
			}
		}
		prev_tbl = ptbl;
		ptbl = ptbl->next;
		ret += CFG_THREAD_TABLE_SIZE;
	}while (ptbl);
	
	/* 重新申请缓冲区 */
	thread_table *new_tbl = new thread_table();
	new_tbl->table[0] = tid;
	wmb();
	
	/* 增加obj_table */
	if (__sync_bool_compare_and_swap(&prev_tbl->next, NULL, new_tbl)) {
		return ret;
	}
	delete new_tbl;

	return tid_reg(tid);
}


int rcu_man::tid_get(void) {
	return tid_reg(pthread_self());
}


void rcu_man::job_start(int tid) {
	obj_table *ptbl = obj_tbl;
	rcu_base *pbase;
	
	/* 遍历所有释放链，调用job_start方法 */
	do
	{
		for (int i=0; i<CFG_OBJ_TABLE_SIZE; i++) {
			/* 搜索结束 */
			pbase = ptbl->table[i];
			if (pbase == NULL) {
				return;
			}
			pbase->job_start(tid);
		}
		ptbl = ptbl->next;
	}while (ptbl);
}


void rcu_man::job_end(int tid) {
	obj_table *ptbl = obj_tbl;
	rcu_base *pbase;
	
	/* 遍历所有释放链，调用job_end方法 */
	do
	{
		for (int i=0; i<CFG_OBJ_TABLE_SIZE; i++) {
			/* 搜索结束 */
			pbase = ptbl->table[i];
			if (pbase == NULL) {
				return;
			}
			pbase->job_end(tid);
		}
		ptbl = ptbl->next;
	}while (ptbl);
}


void rcu_man::job_free(void) {
	obj_table *ptbl = obj_tbl;
	rcu_base *pbase;
	
	/* 遍历所有释放链，调用free方法 */
	do
	{
		for (int i=0; i<CFG_OBJ_TABLE_SIZE; i++) {
			/* 搜索结束 */
			pbase = ptbl->table[i];
			if (pbase == NULL) {
				return;
			}
			pbase->free();
		}
		ptbl = ptbl->next;
	}while (ptbl);
}


void *rcu_man::worker_thread(void *arg) {
	rcu_man *prcu = rcu_man::get_inst();
	while (1) {
		/* 循环调用RCU，释放所有进入Grace Period的对象或者缓冲区 */
		prcu->job_free();
		
		/* 延时 */
		usleep(100*1000);
	}
	return NULL;
}


bool rcu_man::init(void) {
	/* 创建RCU管理线程 */
	pthread_t th_rcu;
	if (pthread_create(&th_rcu, NULL, rcu_man::worker_thread, NULL) < 0) {
		LOGE("Create rcu manage thread failed");
		return false;
	}
	return true;
}
