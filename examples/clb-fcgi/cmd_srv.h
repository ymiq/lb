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
#include <qao/clb_ctl_req.h>
#include <qao/clb_ctl_rep.h>

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

	clb_ctl_rep *company_stat(clb_ctl_req &req);
	clb_ctl_rep *group_stat(clb_ctl_req &req);
	clb_ctl_rep *company_lb(clb_ctl_req &req);
	clb_ctl_rep *group_lb(clb_ctl_req &req);
};


#endif /* __CMD_SRV_H__ */
