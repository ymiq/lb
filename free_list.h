#ifndef _FREE_LIST_H__
#define _FREE_LIST_H__

#include "config.h"

using namespace std; 

class free_list {
public:
	free_list() {};
	~free_list() {};
	virtual void job_start(int id) = 0;
	virtual void job_end(int id) = 0;
	virtual void free(void) = 0;

protected:
	
private:
};


#endif /* _FREE_LIST_H__ */
