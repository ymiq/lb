#ifndef _RCU_MAN_H__
#define _RCU_MAN_H__

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

#define CFG_OBJ_TABLE_SIZE		64
#define CFG_THREAD_TABLE_SIZE	64

typedef struct obj_table {
	free_list *table[CFG_OBJ_TABLE_SIZE];
	struct obj_table *next;
}obj_table;

typedef struct thread_table {
	pthread_t table[CFG_THREAD_TABLE_SIZE];
	struct thread_table *next;
}thread_table;

class rcu_man {
public:
	static rcu_man *get_inst() {
		static rcu_man rcu_singleton;
		return &rcu_singleton;
	};
	bool obj_reg(free_list *obj);
	int thread_reg(void);
	
	int getid(void);
	void job_start(int tid);
	void job_end(int tid);
	void job_free(void);

protected:
	
private:
	obj_table *obj_tbl;
	thread_table *tid_tbl;
	pthread_mutex_t obj_mutex;
	pthread_mutex_t tid_mutex;
	
	rcu_man();
	~rcu_man();
	
	void obj_lock();
	void obj_unlock();

	void tid_lock();
	void tid_unlock();
};

#endif /* _RCU_MAN_H__ */
