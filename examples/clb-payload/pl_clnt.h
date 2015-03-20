#ifndef __PL_CLNT_H__
#define __PL_CLNT_H__

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <evsock.h>
#include <evclnt.h>

using namespace std;

class pl_clnt : public evsock {
public:
	~pl_clnt() {};
	pl_clnt(int fd, struct event_base* evbase) : evsock(fd, evbase) {};
		
	void send_done(void *buf, size_t len, bool send_ok);
	void send_done(qao_base *qao, bool send_ok);
	
	static void read(int sock, short event, void* arg);
	
		
public:

protected:
	
private:
};

extern "C" {
	void dump_receive(void);
}

#endif /* __PL_CLNT_H__ */
