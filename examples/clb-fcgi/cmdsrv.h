#ifndef __CMDSRV_H__
#define __CMDSRV_H__

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <evsock.h>
#include <stat_man.h>
#include <lb_table.h>

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

	void *company_stat(LB_CMD *cmd, size_t *size);
	void *group_stat(LB_CMD *cmd, size_t *size);
	void *company_lb(LB_CMD *cmd, size_t *size);
	void *group_lb(LB_CMD *cmd, size_t *size);
};


#endif /* __CMDSRV_H__ */
