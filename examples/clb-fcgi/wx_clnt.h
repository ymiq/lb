#ifndef __SLB_CLNT_H__
#define __SLB_CLNT_H__

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <evsock.h>
#include <evclnt.h>

using namespace std;

class wx_clnt : public evsock {
public:
	~wx_clnt() {};
	wx_clnt(int fd, struct event_base* evbase) : evsock(fd, evbase) {};
		
	void send_done(void *buf, size_t len, bool send_ok);
	void send_done(qao_base *qao, bool send_ok);
	
	static void read(int sock, short event, void* arg);
	
protected:
	
private:
};

#endif /* __SLB_CLNT_H__ */
