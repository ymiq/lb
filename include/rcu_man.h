#ifndef _RCU_MAN_H__
#define _RCU_MAN_H__

#include <cstdlib>
#include <cstddef>
 
#include <iostream>
#include <list>
#include <numeric>
#include <algorithm>

#include <pthread.h>

#include <config.h>
#include <rcu_base.h>

using namespace std;

class rcu_man {
public:
	static rcu_man *get_inst() {
		static rcu_man rcu_singleton;
		return &rcu_singleton;
	};
	bool obj_reg(rcu_base *obj);
	
	void job_start(int tid);
	void job_end(int tid);
	void job_free(void);

	int tid_get(void);
	static bool init(void);
	protected:
	
private:
	#define CFG_OBJ_TABLE_SIZE		64
	#define CFG_THREAD_TABLE_SIZE	64
	
	typedef struct obj_table {
		rcu_base *table[CFG_OBJ_TABLE_SIZE];
		struct obj_table *next;
	}obj_table;
	
	typedef struct thread_table {
		pthread_t table[CFG_THREAD_TABLE_SIZE];
		struct thread_table *next;
	}thread_table;

	obj_table *obj_tbl;
	thread_table *tid_tbl;
	
	rcu_man();
	~rcu_man();
	
	int tid_reg(pthread_t tid);
	
	static void *worker_thread(void *arg);
};

#endif /* _RCU_MAN_H__ */
