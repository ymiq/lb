#ifndef __COMMAND_H__
#define __COMMAND_H__

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <clb_cmd.h>

using namespace std;

class cmd_clnt {
public:
	cmd_clnt();
	~cmd_clnt();
	
	int request(clb_cmd &cmd);
	int reponse(void);
		
protected:
	
private:
typedef struct LB_CMD {
	unsigned int command;
	unsigned int group;
	unsigned long int hash; 
	unsigned int ip;
	unsigned int port;
	unsigned char data[0];
} LB_CMD;

	int sockfd;
	
	void *do_recv(size_t *len);
	void *get_repsone(size_t *len);
	int do_request(const void *buf, size_t len);
};

#endif /* __COMMAND_H__ */
