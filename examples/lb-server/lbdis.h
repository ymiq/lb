#ifndef __LBDIS_H__
#define __LBDIS_H__

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <pthread.h>
#include <evsock.h>

using namespace std;

class lbdis : public evsock {
public:
	~lbdis();
	lbdis(int fd, struct event_base* base):evsock(fd, base) {}
		
	void ev_send_done(EV_SEND *send);
	
	static void read(int sock, short event, void* arg);
protected:
	
private:
};


#endif /* __LBDIS_H__ */
