#ifndef _OBJ_LIST_H__
#define _OBJ_LIST_H__

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <pthread.h>

#include <iostream>
#include <list>
#include <numeric>
#include <algorithm>

#include "config.h"
#include "free_list.h"

using namespace std;


#define OBJ_LIST_TYPE_CLASS		1
#define OBJ_LIST_TYPE_STRUCT	0
#define OBJ_LIST_TYPE_BUFFER	OBJ_LIST_TYPE_STRUCT


template<typename T> 
class anchor {
public:
	anchor();
	~anchor();
	
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
	int ready;
	int type;		/* 1: obj, 0: buf */
	bool conflict;
	bool peace_map[CFG_MAX_THREADS];
	
	void lock(void);
	void unlock(void);
	
	list<T*> *ready_list(void);
	list<T*> *add_list(void);	
};

template<typename T> 
anchor<T>::anchor() {
	pthread_mutex_init(&mtx, NULL);
	ready = 0;
	type = OBJ_LIST_TYPE_STRUCT;
	conflict = false;
	for (int i=0; i<CFG_MAX_THREADS; i++) {
		peace_map[i] = true;
	}	
}

template<typename T> 
anchor<T>::~anchor() {
}

template<typename T> 
void anchor<T>::lock(void) {
	pthread_mutex_lock(&mtx);
}

template<typename T> 
void anchor<T>::unlock(void) {
	pthread_mutex_unlock(&mtx);
}

template<typename T> 
list<T*> *anchor<T>::ready_list(void) {
	if (ready) {
		return &list1;
	} 
	return &list0;
}

template<typename T> 
list<T*> *anchor<T>::add_list(void) {
	if (ready) {
		return &list0;
	} 
	return &list1;
}

template<typename T> 
void anchor<T>::set_type(int t) {
	type = t;
}

template<typename T> 
void anchor<T>::job_start(int id) {
	if ((id >= 0) && (id < CFG_MAX_THREADS)) {
		peace_map[id] = conflict;
	}
}

template<typename T> 
void anchor<T>::job_end(int id) {
	if ((id >= 0) && (id < CFG_MAX_THREADS)) {
		peace_map[id] = true;
	}
}

template<typename T> 
void anchor<T>::free(void) {	
	/* ���ͷ����� ֱ�ӷ��� */
	if (!conflict) 
		return;
	
	/* ����Ƿ�����ƽʱ�� */
	for (int i=0; i<CFG_MAX_THREADS; i++) {
		if (!peace_map[i])
			return;		
	}
	
	/* ��ȡ������ */
	lock();
	
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
	if (list->empty()) {
		conflict = false;
	} else {
		conflict = true;
	}
	
	/* �ͷŻ����� */
	unlock();
}

template<typename T>
void anchor<T>::add(T *ptr) {
	/* ��ȡ������ */
	lock();
	
	/* ��ӵ��ͷ��б� */
	list<T*> *alist = add_list();
	alist->push_back(ptr);
	
	/* ���ready_list �Ƿ�Ϊ�� */
	list<T*> *rlist = ready_list();
	if (rlist->empty()) {
		ready = ready ? 0: 1;
		conflict = true;
	}
	
	/* �ͷŻ����� */
	unlock();
}


template<typename T> 
class obj_list : virtual free_list {
public:
	obj_list() {};
	~obj_list() {};
	
	void job_start(int id);
	void job_end(int id);
	void free(void);
	void add(T *ptr);
	void set_type(int type);

protected:
	
private:
	static anchor<T> obj;
};

/* ��̬���ݳ�Ա�ĳ�ʼ�� */
template <typename T>
struct anchor<T> obj_list<T>::obj;


template<typename T>
void obj_list<T>::job_start(int id) {
	obj.job_start(id);
}

template<typename T>
void obj_list<T>::job_end(int id) {
	obj.job_end(id);
}

template<typename T>
void obj_list<T>::free(void) {
	obj.free();
}

template<typename T>
void obj_list<T>::add(T *ptr) {
	obj.add(ptr);
}

template<typename T>
void obj_list<T>::set_type(int type) {
	obj.set_type(type);
}

#endif /* _OBJ_LIST_H__ */
