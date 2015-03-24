#ifndef __EV_OBJ_H__
#define __EV_OBJ_H__

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>

using namespace std;

class evobj {
public:
	evobj() {};
	virtual ~evobj() {};
		
	virtual void ev_close() {};
	
protected:
	
private:
};

#endif /* __EV_OBJ_H__ */
