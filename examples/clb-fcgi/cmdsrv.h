#ifndef __CMDSRV_H__
#define __CMDSRV_H__

#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <string>
#include <evsock.h>
#include <stat_man.h>
#include <lb_table.h>
#include <clb_cmd.h>

using namespace std;

typedef struct LB_CMD {
	unsigned int cmd;
	unsigned int group;
	unsigned long int hash; 
	unsigned int ip;
	unsigned int port;
	unsigned char data[0];
} LB_CMD;

class cmdsrv : public evsock {
public:
	~cmdsrv();
	cmdsrv(int fd, struct event_base* base);
		
	void send_done(unsigned long int token, void *buf, size_t len);
	
	static void read(int sock, short event, void* arg);
protected:
	
private:
	stat_man *pstat;
	lb_table *plb;

	clb_cmd_resp *company_stat(clb_cmd &cmd);
	clb_cmd_resp *group_stat(clb_cmd &cmd);
	clb_cmd_resp *company_lb(clb_cmd &cmd);
	clb_cmd_resp *group_lb(clb_cmd &cmd);
};


#endif /* __CMDSRV_H__ */
