
#include <string.h>
#include "rcu_man.h"

rcu_man::rcu_man() {
	obj_tbl = (obj_table*) calloc(sizeof(obj_table), 1);
	if (obj_tbl  == NULL) {
		throw "No memory to construct rcu_man";
	}
	tid_tbl = (thread_table*) calloc(sizeof(thread_table), 1);
	if (tid_tbl  == NULL) {
		throw "No memory to construct rcu_man";
	}
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


bool rcu_man::obj_reg(free_list *obj) {
	obj_lock(); 
	obj_table *ptbl = obj_tbl;
	obj_table *prev_tbl;
	
	/* 等级指针 */
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
	obj_table *new_tbl = (obj_table*) calloc(sizeof(obj_table), 1);
	if (new_tbl  == NULL) {
		throw "No memory to construct rcu_man";
	}
	new_tbl->table[0] = obj;
	
	/* 避免乱序造成问题 */
	mb();
	prev_tbl->next = new_tbl;
	
	obj_unlock();
	return true;
}

int rcu_man::thread_reg(void) {
	pthread_t tid = pthread_self();	
	int ret;
	
	tid_lock();  
	
	thread_table *ptbl = tid_tbl;
	thread_table *prev_tbl;
	
	/* 等级指针 */
	ret = 0;
	do
	{
		for (int i=0; i<CFG_THREAD_TABLE_SIZE; i++) {
			if (ptbl->table[i] == NULL) {
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
	thread_table *new_tbl = (thread_table*) calloc(sizeof(thread_table), 1);
	if (new_tbl  == NULL) {
		throw "No memory to construct rcu_man";
	}
	new_tbl->table[0] = tid;
	
	/* 避免乱序造成问题 */
	mb();
	prev_tbl->next = new_tbl;
			
	tid_unlock();
	return ret;
}

int rcu_man::getid(void) {
	int ret = 0;
	pthread_t tid = pthread_self();	
	thread_table *ptbl = tid_tbl;
	
	/* 遍历所有线程表，获取线程ID */
	do
	{
		for (int i=0; i<CFG_THREAD_TABLE_SIZE; i++) {
			/* 搜索结束 */
			if (!ptbl->table[i])
				return -1;
			
			/* 找到结果 */
			if (ptbl->table[i] == tid) {
				return ret + i;
			}
		}
		ptbl = ptbl->next;
		ret += CFG_THREAD_TABLE_SIZE;
	}while (ptbl);
	return -1;
}

void rcu_man::job_start(void) {
	obj_table *ptbl = obj_tbl;
	
	/* 遍历所有释放链，调用job_start方法 */
	do
	{
		for (int i=0; i<CFG_OBJ_TABLE_SIZE; i++) {
			/* 搜索结束 */
			if (ptbl->table[i] == NULL) {
				return;
			}
			ptbl->table[i].job_start();
		}
		ptbl = ptbl->next;
	}while (ptbl);
}

void rcu_man::job_end(void) {
	obj_table *ptbl = obj_tbl;
	
	/* 遍历所有释放链，调用job_end方法 */
	do
	{
		for (int i=0; i<CFG_OBJ_TABLE_SIZE; i++) {
			/* 搜索结束 */
			if (ptbl->table[i] == NULL) {
				return;
			}
			ptbl->table[i].job_end();
		}
		ptbl = ptbl->next;
	}while (ptbl);
}

void rcu_man::free_thread(void) {
}

