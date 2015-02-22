#ifndef _RCU_BASE_H__
#define _RCU_BASE_H__

#include "config.h"

using namespace std; 

class rcu_base {
public:
	rcu_base() {};
	~rcu_base() {};
	virtual void job_start(int id) = 0;
	virtual void job_end(int id) = 0;
	virtual void free(void) = 0;

protected:
	
private:
};


#endif /* _RCU_BASE_H__ */
