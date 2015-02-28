#ifndef __EVSOCK_H__
#define __EVSOCK_H__

#include <cstdlib>
#include <cstddef>

#include <config.h>

using namespace std;

class evsock {
public:
	evsock();
	~evsock();
    struct event read_ev;
    struct event write_ev;
	
	virtual read();
	virtual write();
protected:
	
private:
	int sockfd;
}
	
#endif /* __EVSOCK_H__ */ 