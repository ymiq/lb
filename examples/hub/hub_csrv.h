#ifndef __HUB_CLB_SRV_H__
#define __HUB_CLB_SRV_H__

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <pthread.h>
#include <evsock.h>
#include <qao/qao_base.h>

using namespace std;

class hub_csrv : public evsock {
public:
	~hub_csrv();
	hub_csrv(int fd, struct event_base* base):evsock(fd, base) {}
		
	void send_done(void *buf, size_t len, bool send_ok);
	void send_done(qao_base *qao, bool send_ok) {};
	
	static void read(int sock, short event, void* arg);
protected:
	
private:
};


#endif /* __LBDIS_H__ */
