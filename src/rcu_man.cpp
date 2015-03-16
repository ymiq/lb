
#include <cstring>
#include <utils/log.h>
#include <rcu_man.h>
#include <unistd.h>

rcu_man::rcu_man() {
	obj_tbl = new obj_table();
	tid_tbl = new thread_table();
	pthread_mutex_init(&obj_mutex, NULL);
	pthread_mutex_init(&tid_mutex, NULL);
}


rcu_man::~rcu_man() {
}


void rcu_man::obj_lock() {
	pthread_mutex_lock(&obj_mutex);
}


void rcu_man::obj_unlock() {
	pthread_mutex_unlock(&obj_mutex);
}


void rcu_man::tid_lock() {
	pthread_mutex_lock(&tid_mutex);
}


void rcu_man::tid_unlock() {
	pthread_mutex_unlock(&tid_mutex);
}


bool rcu_man::obj_reg(rcu_base *obj) {
	obj_lock(); 
	obj_table *ptbl = obj_tbl;
	obj_table *prev_tbl;
	
	/* 检查释放已经注册 */
	do
	{
		for (int i=0; i<CFG_OBJ_TABLE_SIZE; i++) {
			if (ptbl->table[i] == obj) {
				obj_unlock();
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
			if (ptbl->table[i] == NULL) {
				ptbl->table[i] = obj;
				obj_unlock();
				return true;
			}
		}
		prev_tbl = ptbl;
		ptbl = ptbl->next;
	}while (ptbl);
	
	/* 重新申请缓冲区 */
	obj_table *new_tbl = new obj_table();
	new_tbl->table[0] = obj;
	
	/* 避免乱序造成问题 */
	wmb();
	prev_tbl->next = new_tbl;
	
	obj_unlock();
	return true;
}


int rcu_man::tid_reg(pthread_t tid) {
	int ret;
	
	tid_lock();  
	
	thread_table *ptbl = tid_tbl;
	thread_table *prev_tbl;
	
	/* 检查线程是否已经注册 */
	ret = 0;
	do
	{
		for (int i=0; i<CFG_THREAD_TABLE_SIZE; i++) {
			if (ptbl->table[i] == tid) {
				obj_unlock();
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
			if (ptbl->table[i] == 0) {
				ptbl->table[i] = tid;
				obj_unlock();
				return ret + i;
			}
		}
		prev_tbl = ptbl;
		ptbl = ptbl->next;
		ret += CFG_THREAD_TABLE_SIZE;
	}while (ptbl);
	
	/* 重新申请缓冲区 */
	thread_table *new_tbl = new thread_table();
	new_tbl->table[0] = tid;
	
	/* 避免乱序造成问题 */
	wmb();
	prev_tbl->next = new_tbl;
			
	tid_unlock();
	return ret;
}


int rcu_man::tid_get(void) {
	int ret = 0;
	pthread_t tid = pthread_self();	
	thread_table *ptbl = tid_tbl;
	pthread_t save_tid;
	
	/* 遍历所有线程表，获取线程ID */
	do
	{
		for (int i=0; i<CFG_THREAD_TABLE_SIZE; i++) {
			/* 搜索结束 */
			save_tid = ptbl->table[i];
			if (!save_tid) {
				return tid_reg(tid);
			}
			
			/* 找到结果 */
			if (save_tid == tid) {
				return ret + i;
			}
		}
		ptbl = ptbl->next;
		ret += CFG_THREAD_TABLE_SIZE;
	}while (ptbl);
	return -1;
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
