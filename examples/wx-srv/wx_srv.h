#ifndef __CLB_SRV_H__
#define __CLB_SRV_H__

#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <string>
#include <evsock.h>
#include <qao/qao_base.h>
#include <rcu_man.h>

using namespace std;

class wx_srv : public evsock {
public:
	~wx_srv();
	wx_srv(int fd, struct event_base* base);
		
	void send_done(void *buf, size_t len, bool send_ok);
	void send_done(qao_base *qao, bool send_ok);
	
	void dump_performance(unsigned long reply);
	
	static void read(int sock, short event, void* arg);
protected:
	
private:
	struct timeval start_tv;
	unsigned long recv_questions;
};

extern "C" {
	extern bool question_trace;
};

#endif /* __CLB_SRV_H__ */
