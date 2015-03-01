
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>

#include <unistd.h>
#include <log.h>
#include "lbdis.h"

using namespace std;

typedef struct LB_CMD {
	unsigned int command;
	unsigned int group;
	unsigned long int hash; 
	unsigned int ip;
	unsigned int port;
	unsigned char data[0];
} LB_CMD;

lbdis::~lbdis() {
}

void lbdis::read(int sock, short event, void* arg) {
	lbdis *srv = (lbdis *)arg;
	void *buffer;
	size_t size = 0;
	
	/* 接收数据 */
	buffer = srv->ev_recv(&size);
	if ((int)size <= 0) {
		/* = 0: 客户端断开连接，在这里移除读事件并且释放客户数据结构 */
		/* < 0: 出现了其它的错误，在这里关闭socket，移除事件并且释放客户数据结构 */
		delete srv;
		return;
	}
	
    /* 处理数据 */
#if 1
    LB_CMD *pcmd = (LB_CMD *) buffer;
    printf("Get command\n");
    printf("  CMD: 0x%08x\n", pcmd->command);
    printf(" HASH: 0x%lx\n", pcmd->hash);
    printf("GROUP: %d\n", pcmd->group);
    printf("   IP: 0x%08x\n", pcmd->ip);
    printf(" PORT: %d\n", pcmd->port);
    printf("\n");
    
    EV_SEND send;
    
    send.token = 0;
    send.buf = buffer;
    send.size = 16;
    *(bool*)buffer = true;
    if (srv->ev_send(&send) == false) {
    	perror("reponse error");
    }
    
#else
    buffer[256] = '\0';
    syslog(LOG_INFO, "GET QUESTION: %s\n", buffer);
#endif
    
	/* 释放缓冲区 */
	srv->ev_recv_done(buffer);
}

void lbdis::ev_send_done(EV_SEND *send) {
	/* 释放发送缓冲区 */
	free(send);
//	free(send->buf);
}
