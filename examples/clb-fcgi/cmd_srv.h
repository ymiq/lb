#ifndef __CMD_SRV_H__
#define __CMD_SRV_H__

#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <string>
#include <evsock.h>
#include <stat_man.h>
#include <clb_tbl.h>
#include <clb_grp.h>
#include <qao/cctl_req.h>
#include <qao/qao_base.h>

using namespace std;

class cmd_srv : public evsock {
public:
	~cmd_srv();
	cmd_srv(int fd, struct event_base* base);
		
	void send_done(void *buf, size_t len, bool send_ok);
	void send_done(qao_base *rep, bool send_ok);
	
	static void read(int sock, short event, void* arg);
protected:
	
private:
	stat_man *pstat;
	clb_tbl *plb;
	clb_grp *pgrp;

	qao_base *company_stat(cctl_req &req);
	qao_base *group_stat(cctl_req &req);
	qao_base *company_lb(cctl_req &req);
	qao_base *group_lb(cctl_req &req);
};


#endif /* __CMD_SRV_H__ */
