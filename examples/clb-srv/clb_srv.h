#ifndef __CLB_SRV_H__
#define __CLB_SRV_H__

#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <string>
#include <evsock.h>
#include <clb_tbl.h>
#include <clb_grp.h>
#include <qao/qao_base.h>
#include <stat/stat_man.h>
#include <stat/stat_tbl.h>
#include <rcu_man.h>

using namespace std;

class clb_srv : public evsock {
public:
	~clb_srv();
	clb_srv(int fd, struct event_base* base);
		
	void send_done(void *buf, size_t len, bool send_ok);
	void send_done(qao_base *qao, bool send_ok);
	
	static void read(int sock, short event, void* arg);
protected:
	
private:
	clb_tbl *plb;
	clb_grp *pgrp;
	rcu_man *prcu;
	int tid;
};

extern "C" {
	void qao_srv_bind(qao_base *qao, clb_srv *srv);
	void qao_srv_unbind(qao_base *qao);
};

#endif /* __CLB_SRV_H__ */
