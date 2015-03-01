
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>

#include <unistd.h>
#include <log.h>
#include "cmdsrv.h"

using namespace std;

typedef struct LB_CMD {
	unsigned int command;
	unsigned int group;
	unsigned long int hash; 
	unsigned int ip;
	unsigned int port;
	unsigned char data[0];
} LB_CMD;

cmdsrv::~cmdsrv() {
}

void cmdsrv::read(int sock, short event, void* arg) {
	cmdsrv *srv = (cmdsrv *)arg;
	void *buffer;
	char *resp_buf;
	size_t size;
	
	/* 接收数据 */
	buffer = srv->ev_recv(&size);
	if ((int)size <= 0) {
		/* = 0: 客户端断开连接，在这里移除读事件并且释放客户数据结构 */
		/* < 0: 出现了其它的错误，在这里关闭socket，移除事件并且释放客户数据结构 */
		delete srv;
		return;
	}
	
	/* 处理命令 */
    LB_CMD *pcmd = (LB_CMD *) buffer;
    LOGD("Get command");
    LOGD("  CMD: 0x%08x", pcmd->command);
    LOGD(" HASH: 0x%lx", pcmd->hash);
    LOGD("GROUP: %d", pcmd->group);
    LOGD("   IP: 0x%08x", pcmd->ip);
    LOGD(" PORT: %d", pcmd->port);
    
    EV_SEND send;
    send.token = 0;
    send.buf = malloc(1024);
    if (send.buf == NULL) {
    	LOGE("no memory");
    	goto out;
    }
    send.size = 16;
    *(bool*)send.buf = true;
    if (srv->ev_send(&send) == false) {
    	LOGE("reponse error");
    }
	
	/* 释放缓冲区 */
out:
	srv->ev_recv_done(buffer);
}

void cmdsrv::ev_send_done(EV_SEND *send) {
	/* 释放发送缓冲区 */
	free(send->buf);
	free(send);
}
