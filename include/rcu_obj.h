/*
 * RCU对象，用于处理类似share_ptr问题。
 * 对多线程共享对象/缓冲区指针，使用延时释放的方法，而不是对象引用计数器。
 *
 */

#ifndef _RCU_OBJ_H__
#define _RCU_OBJ_H__

#include <cstdlib>
#include <cstddef>

#include <iostream>
#include <list>
#include <numeric>
#include <algorithm>

#include <pthread.h>
#include <config.h>
#include <utils/log.h>
#include <rcu_base.h>
#include <rcu_man.h>

using namespace std;


#define OBJ_LIST_TYPE_CLASS			0
#define OBJ_LIST_TYPE_CLASS_ARRAY	1
#define OBJ_LIST_TYPE_STRUCT		2
#define OBJ_LIST_TYPE_BUFFER	OBJ_LIST_TYPE_STRUCT


template<typename T> 
class rcu_instance : virtual rcu_base {
public:
	rcu_instance(int threads=0);
	~rcu_instance();
	
	/* 每个线程在主业务处理前、后分别调用job_start和job_end函数。调用简单实例：
	 * while (1) {
	 * 	select(...);
	 * 	
	 * 	// 开启RCU安全期检查
	 * 	job_start();
	 * 	
	 * 	// 主业务处理
	 * 	job_do.....
	 * 	
	 * 	// 结束RCU安全期检查
	 * 	job_end();
	 * }
	 */
	void job_start(int id);
	void job_end(int id);
	
	void free(void);	
	void add(T *ptr);
	
	void reg(int t=OBJ_LIST_TYPE_CLASS);

protected:
	
private:
	pthread_mutex_t mtx;
	list<T*> list0;
	list<T*> list1;
	int ready;		/* 指向待释放对象/缓冲区链(list0 or list1) */
	int type;		/* 释放类型：1: obj, 0: buf */
	bool conflict;	/* TRUE:  存在释放对象/缓冲区，开始冲突检测 */
					/* FALSE: 结束冲突检测 */
					
	/* 每线程变量。静止状态，指示当前线程是否进入Grace period.
	 * 值： 0 - 存在冲突可能，不能释放
	 *      1 - Grace Period，可以释放
	 *      2 - Grace Period，conflict变为真时失效
	 * 
	 * 状态：
	 * 1. 主业务处理之后（调用job_end)，当前线程立即进入Grace period（值为：1）
	 * 2. 主业务处理之前（调用job_start）未冲突检测，主处理业务过程中不能进入Grace period（值为：0）
	 *    主业务处理中开启冲突检测，当前线程无法进入Grace period，只有等待下一次主处理周期
	 * 3. 主业务处理之前（调用job_start）开启冲突检测，当前线程立即进入Grace period（值为：2）
	 *    主业务处理中，新启了一次安全检测，进入Grace period（值为：2）的线程，
	 *    将退出Grace Period（值为: 0)，需要等待下一次主处理周期才能进入Grace period
	 */
	int *quiescent;
	int max_threads;

	void lock(void);
	void unlock(void);
	
	list<T*> *ready_list(void);
	list<T*> *append_list(void);	
};

template<typename T> 
rcu_instance<T>::rcu_instance(int threads) {
	
	/* 全局变量初始化 */
	pthread_mutex_init(&mtx, NULL);
	ready = 0;
	type = OBJ_LIST_TYPE_CLASS;
	conflict = false;
	if (threads <= 0) {
		threads = CFG_MAX_THREADS;
	}
	max_threads = threads;
	quiescent = new int[threads]();
	for (int i=0; i<threads; i++) {
		quiescent[i] = 1;
	}
}

template<typename T> 
rcu_instance<T>::~rcu_instance() {
	pthread_mutex_destroy(&mtx);
	delete[] quiescent;
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
list<T*> *rcu_instance<T>::append_list(void) {
	if (ready) {
		return &list0;
	} 
	return &list1;
}

template<typename T> 
void rcu_instance<T>::reg(int t) {
	
	/* 注册RCU结构体/对象到RCU管理器 */
	rcu_man *prcu = rcu_man::get_inst();
	if (prcu == NULL) {
		throw "Can't get instance of rcu_man";
	}
	prcu->obj_reg(this);
	
	/* 设置类型 */
	type = t;
}

template<typename T> 
void rcu_instance<T>::job_start(int id) {
	if ((id >= 0) && (id < max_threads)) {
		if (conflict) {
			quiescent[id] = 2;
		} else {
			quiescent[id] = 0;
		}
	}
}

template<typename T> 
void rcu_instance<T>::job_end(int id) {
	if ((id >= 0) && (id < max_threads)) {
		quiescent[id] = 1;
	}
}


template<typename T> 
void rcu_instance<T>::free(void) {	
	/* 无释放任务， 直接返回 */
	if (!conflict) 
		return;
	
	/* 获取互斥量 */
	lock();
	
	/* 检查是否进入和平时期 */
	for (int i=0; i<max_threads; i++) {
		if (!quiescent[i]) {
			unlock();
			return;		
		}
	}
	
	/* 释放对象 */
	list<T*> *list = ready_list();
	typename list<T*>::iterator element;
	for (element=list->begin(); element!=list->end(); ++element) {
		if (type == OBJ_LIST_TYPE_CLASS) {
			delete (T*) (*element);
		} else if (type == OBJ_LIST_TYPE_CLASS_ARRAY) {
			delete[] (T*) (*element);
		}else {
			std::free(*element);
		}
	}
	list->clear();
	
	/* 切换Ready链表 */
	ready = ready ? 0 : 1;
	
	/* 检查是否存在需要释放的内容 */
	list = ready_list();
	if (list->empty()) {
		conflict = false;
	} else {
		/* 进入Grace Period（值为：2）的线程，将强制退出Grace Period */
		for (int i=0; i<max_threads; i++) {
			__sync_bool_compare_and_swap(&quiescent[i], 2, 0);
		}
		
		wmb();
		conflict = true;
	}
	
	/* 释放互斥量 */
	unlock();
}

template<typename T>
void rcu_instance<T>::add(T *ptr) {
	/* 检查参数判断 */
	if (!ptr) {
		return;
	}
	
	/* 获取互斥量 */
	lock();
	
	/* 添加到释放列表 */
	list<T*> *alist = append_list();
	alist->push_back(ptr);
	
	/* 检查ready_list 是否为空 */
	list<T*> *rlist = ready_list();
	if (rlist->empty()) {
		ready = ready ? 0 : 1;
		
		/* 进入Grace Period（值为：2）的线程，将强制退出Grace Period */
		for (int i=0; i<max_threads; i++) {
			__sync_bool_compare_and_swap(&quiescent[i], 2, 0);
		}
		
		wmb();
		conflict = true;
	}
	
	/* 释放互斥量 */
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
	void reg(int type=OBJ_LIST_TYPE_CLASS);

protected:
	
private:
	static rcu_instance<T> obj;
};

/* 静态数据成员的初始化 */
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
void rcu_obj<T>::reg(int type) {
	obj.reg(type);
}

#endif /* _RCU_OBJ_H__ */
