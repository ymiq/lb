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

class clbsrv : public evsock {
public:
	~clbsrv();
	clbsrv(int fd, struct event_base* base):evsock(fd, base) {}
		
	void send_done(unsigned long token, void *buf, size_t len);
	
	static void read(int sock, short event, void* arg);
protected:
	
private:
};


#endif /* __LBDIS_H__ */
