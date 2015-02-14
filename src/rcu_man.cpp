
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
	
	/* ����ͷ��Ѿ�ע�� */
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
	
	/* ע��ָ�� */
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
	
	/* �������뻺���� */
	obj_table *new_tbl = (obj_table*) calloc(sizeof(obj_table), 1);
	if (new_tbl  == NULL) {
		throw "No memory to construct rcu_man";
	}
	new_tbl->table[0] = obj;
	
	/* ��������������� */
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
	
	/* ����߳��Ƿ��Ѿ�ע�� */
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
	
	/* ע���߳� */
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
	
	/* �������뻺���� */
	thread_table *new_tbl = (thread_table*) calloc(sizeof(thread_table), 1);
	if (new_tbl  == NULL) {
		throw "No memory to construct rcu_man";
	}
	new_tbl->table[0] = tid;
	
	/* ��������������� */
	mb();
	prev_tbl->next = new_tbl;
			
	tid_unlock();
	return ret;
}

int rcu_man::getid(void) {
	int ret = 0;
	pthread_t tid = pthread_self();	
	thread_table *ptbl = tid_tbl;
	
	/* ���������̱߳���ȡ�߳�ID */
	do
	{
		for (int i=0; i<CFG_THREAD_TABLE_SIZE; i++) {
			/* �������� */
			if (!ptbl->table[i])
				return -1;
			
			/* �ҵ���� */
			if (ptbl->table[i] == tid) {
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
	
	/* ���������ͷ���������job_start���� */
	do
	{
		for (int i=0; i<CFG_OBJ_TABLE_SIZE; i++) {
			/* �������� */
			if (ptbl->table[i] == NULL) {
				return;
			}
			ptbl->table[i]->job_start(tid);
		}
		ptbl = ptbl->next;
	}while (ptbl);
}

void rcu_man::job_end(int tid) {
	obj_table *ptbl = obj_tbl;
	
	/* ���������ͷ���������job_end���� */
	do
	{
		for (int i=0; i<CFG_OBJ_TABLE_SIZE; i++) {
			/* �������� */
			if (ptbl->table[i] == NULL) {
				return;
			}
			ptbl->table[i]->job_end(tid);
		}
		ptbl = ptbl->next;
	}while (ptbl);
}

void rcu_man::job_free(void) {
	obj_table *ptbl = obj_tbl;
	
	/* ���������ͷ���������free���� */
	do
	{
		for (int i=0; i<CFG_OBJ_TABLE_SIZE; i++) {
			/* �������� */
			if (ptbl->table[i] == NULL) {
				return;
			}
			ptbl->table[i]->free();
		}
		ptbl = ptbl->next;
	}while (ptbl);
}

