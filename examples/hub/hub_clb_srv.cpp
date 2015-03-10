
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>

#include <unistd.h>
#include <log.h>
#include "hub_clb_srv.h"

using namespace std;

hub_clb_srv::~hub_clb_srv() {
}

void hub_clb_srv::read(int sock, short event, void* arg) {
	hub_clb_srv *srv = (hub_clb_srv *)arg;
	void *buffer;
	size_t size = 0;
	
	/* 接收数据 */
	buffer = srv->ev_recv(size);
	if ((int)size <= 0) {
		/* = 0: 客户端断开连接，在这里移除读事件并且释放客户数据结构 */
		/* < 0: 出现了其它的错误，在这里关闭socket，移除事件并且释放客户数据结构 */
		delete srv;
		return;
	}
	
    /* 处理数据 */
    char *request = (char*)buffer;
    request[256] = '\0';
//    syslog(LOG_INFO, "GET QUESTION: %s\n", buffer);
	printf("GET QUESTION: %s\n", request);
    
	/* 释放缓冲区 */
	srv->recv_done(buffer);
}


void hub_clb_srv::send_done(void *buf, size_t len, bool send_ok) {
	/* 释放发送缓冲区 */
//	free(send->buf);
}

