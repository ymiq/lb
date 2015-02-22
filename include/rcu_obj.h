#ifndef _RCU_OBJ_H__
#define _RCU_OBJ_H__

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <pthread.h>

#include <iostream>
#include <list>
#include <numeric>
#include <algorithm>

#include "config.h"
#include "rcu_base.h"
#include "rcu_man.h"

using namespace std;


#define OBJ_LIST_TYPE_CLASS		1
#define OBJ_LIST_TYPE_STRUCT	0
#define OBJ_LIST_TYPE_BUFFER	OBJ_LIST_TYPE_STRUCT


template<typename T> 
class rcu_instance {
public:
	rcu_instance();
	~rcu_instance();
	
	/* ÿ���߳�����ҵ����ǰ����ֱ����job_start��job_end���������ü�ʵ����
	 * while (1) {
	 * 	select(...);
	 * 	
	 * 	// ����RCU��ȫ�ڼ��
	 * 	job_start();
	 * 	
	 * 	// ��ҵ����
	 * 	job_do.....
	 * 	
	 * 	// ����RCU��ȫ�ڼ��
	 * 	job_end();
	 * }
	 */
	void job_start(int id);
	void job_end(int id);
	
	void free(void);	
	void add(T *ptr);
	
	void set_type(int t);

protected:
	
private:
	pthread_mutex_t mtx;
	list<T*> list0;
	list<T*> list1;
	int ready;		/* ָ����ͷŶ���/��������(list0 or list1) */
	int type;		/* �ͷ����ͣ�1: obj, 0: buf */
	bool conflict;	/* TRUE:  �����ͷŶ���/����������ʼ��ͻ��� */
					/* FALSE: ������ͻ��� */
					
	/* ÿ�̱߳�������ֹ״̬��ָʾ��ǰ�߳��Ƿ����Grace period.
	 * ֵ�� 0 - ���ڳ�ͻ���ܣ������ͷ�
	 *      1 - Grace Period�������ͷ�
	 *      2 - Grace Period��conflict��Ϊ��ʱʧЧ
	 * 
	 * ״̬��
	 * 1. ��ҵ����֮�󣨵���job_end)����ǰ�߳���������Grace period��ֵΪ��1��
	 * 2. ��ҵ����֮ǰ������job_start��δ��ͻ��⣬������ҵ������в��ܽ���Grace period��ֵΪ��0��
	 *    ��ҵ�����п�����ͻ��⣬��ǰ�߳��޷�����Grace period��ֻ�еȴ���һ������������
	 * 3. ��ҵ����֮ǰ������job_start��������ͻ��⣬��ǰ�߳���������Grace period��ֵΪ��2��
	 *    ��ҵ�����У�������һ�ΰ�ȫ��⣬����Grace period��ֵΪ��2�����̣߳�
	 *    ���˳�Grace Period��ֵΪ: 0)����Ҫ�ȴ���һ�����������ڲ��ܽ���Grace period
	 */
	int quiescent[CFG_MAX_THREADS];

	void lock(void);
	void unlock(void);
	
	list<T*> *ready_list(void);
	list<T*> *add_list(void);	
};

template<typename T> 
rcu_instance<T>::rcu_instance() {
	
	/* ȫ�ұ�����ʼ�� */
	pthread_mutex_init(&mtx, NULL);
	ready = 0;
	type = OBJ_LIST_TYPE_STRUCT;
	conflict = false;
	for (int i=0; i<CFG_MAX_THREADS; i++) {
		quiescent[i] = true;
	}
	
	/* ע��RCU�ṹ��/����RCU������ */
	rcu_man *prcu = rcu_man::get_inst();
	if (prcu == NULL) {
		throw "Can't get instance of rcu_man";
	}
	prcu->obj_reg((rcu_base*)this);
}

template<typename T> 
rcu_instance<T>::~rcu_instance() {
}

template<typename T> 
void rcu_instance<T>::lock(void) {
	pthread_mutex_lock(&mtx);
}

template<typename T> 
void rcu_instance<T>::unlock(void) {
	pthread_mutex_unlock(&mtx);
}

template<typename T> 
list<T*> *rcu_instance<T>::ready_list(void) {
	if (ready) {
		return &list1;
	} 
	return &list0;
}

template<typename T> 
list<T*> *rcu_instance<T>::add_list(void) {
	if (ready) {
		return &list0;
	} 
	return &list1;
}

template<typename T> 
void rcu_instance<T>::set_type(int t) {
	type = t;
}

template<typename T> 
void rcu_instance<T>::job_start(int id) {
	if ((id >= 0) && (id < CFG_MAX_THREADS)) {
		if (conflict) {
			quiescent[id] = 2;
		} else {
			quiescent[id] = 0;
		}
	}
}

template<typename T> 
void rcu_instance<T>::job_end(int id) {
	if ((id >= 0) && (id < CFG_MAX_THREADS)) {
		quiescent[id] = 1;
	}
}

template<typename T> 
void rcu_instance<T>::free(void) {	
	/* ���ͷ����� ֱ�ӷ��� */
	if (!conflict) 
		return;
	
	/* ��ȡ������ */
	lock();
	
	/* ����Ƿ�����ƽʱ�� */
	for (int i=0; i<CFG_MAX_THREADS; i++) {
		if (!quiescent[i]) {
			unlock();
			return;		
		}
	}
	
	/* �ͷŶ��� */
	list<T*> *list = ready_list();
	typename list<T*>::iterator element;
	for (element=list->begin(); element!=list->end(); ++element) {
		if (type) {
			delete (T*) (*element);
		} else {
			std::free(*element);
		}
	}
	list->clear();
	
	/* �л�Ready���� */
	ready = ready ? 0: 1;
	
	/* ����Ƿ������Ҫ�ͷŵ����� */
	list = ready_list();
	mb();
	if (list->empty()) {
		conflict = false;
	} else {
		/* ����Grace Period��ֵΪ��2�����̣߳���ǿ���˳�Grace Period */
		for (int i=0; i<CFG_MAX_THREADS; i++) {
			__sync_val_compare_and_swap(&quiescent[i], 2, 0);
		}
		
		wmb();
		conflict = true;
	}
	
	/* �ͷŻ����� */
	unlock();
}

template<typename T>
void rcu_instance<T>::add(T *ptr) {
	/* ��ȡ������ */
	lock();
	
	/* ��ӵ��ͷ��б� */
	list<T*> *alist = add_list();
	alist->push_back(ptr);
	
	/* ���ready_list �Ƿ�Ϊ�� */
	list<T*> *rlist = ready_list();
	if (rlist->empty()) {
		ready = ready ? 0: 1;
		
		/* ����Grace Period��ֵΪ��2�����̣߳���ǿ���˳�Grace Period */
		for (int i=0; i<CFG_MAX_THREADS; i++) {
			__sync_val_compare_and_swap(&quiescent[i], 2, 0);
		}
		
		wmb();
		conflict = true;
	}
	
	/* �ͷŻ����� */
	unlock();
}


template<typename T> 
class rcu_obj : virtual rcu_base {
public:
	rcu_obj() {};
	~rcu_obj() {};
	
	void job_start(int id);
	void job_end(int id);
	void free(void);
	void add(T *ptr);
	void set_type(int type);

protected:
	
private:
	static rcu_instance<T> obj;
};

/* ��̬���ݳ�Ա�ĳ�ʼ�� */
template <typename T>
struct rcu_instance<T> rcu_obj<T>::obj;


template<typename T>
void rcu_obj<T>::job_start(int id) {
	obj.job_start(id);
}

template<typename T>
void rcu_obj<T>::job_end(int id) {
	obj.job_end(id);
}

template<typename T>
void rcu_obj<T>::free(void) {
	obj.free();
}

template<typename T>
void rcu_obj<T>::add(T *ptr) {
	obj.add(ptr);
}

template<typename T>
void rcu_obj<T>::set_type(int type) {
	obj.set_type(type);
}

#endif /* _RCU_OBJ_H__ */
