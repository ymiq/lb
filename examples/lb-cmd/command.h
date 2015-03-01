#ifndef __COMMAND_H__
#define __COMMAND_H__

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>

using namespace std;

class command {
public:
	command();
	~command();
	
	int request(unsigned int cmd, unsigned long int hash, unsigned int id,
					unsigned int ip, unsigned int port);
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
	
	int do_request(void *buf, size_t len);
	int get_repsone(void *buf, int *size);
};

#endif /* __COMMAND_H__ */
