#ifndef __CMD_CLNT_H__
#define __CMD_CLNT_H__

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <evsock.h>
#include <evclnt.h>
#include <qao/cctl_req.h>
#include <qao/cctl_rep.h>

using namespace std;

class cmd_clnt : public evsock {
public:
	~cmd_clnt() {};
	cmd_clnt(int fd, struct event_base* evbase) : evsock(fd, evbase), qao(NULL) {base = evbase;};
		
	void send_done(void *buf, size_t len, bool send_ok);
	void send_done(qao_base *qao, bool send_ok);
	
	static void read(int sock, short event, void* arg);
	static void timer_cb(int sock, short event, void* arg);
	
	bool open_timer(void);
	void reg_qao(qao_base *q) {qao = q;};
		
protected:
	
private:
	struct event_base* base;
	qao_base *qao;
	struct event ev;
	struct timeval tv;
};

#endif /* __CMD_CLNT_H__ */
