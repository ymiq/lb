
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>

#include <unistd.h>
#include <log.h>
#include "clbsrv.h"

using namespace std;

clbsrv::~clbsrv() {
}

void clbsrv::read(int sock, short event, void* arg) {
	clbsrv *srv = (clbsrv *)arg;
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
//    buffer[256] = '\0';
//    syslog(LOG_INFO, "GET QUESTION: %s\n", buffer);
    
	/* 释放缓冲区 */
	srv->recv_done(buffer);
}

void clbsrv::send_done(unsigned long int token, void *buf, size_t len) {
	/* 释放发送缓冲区 */
//	free(send->buf);
}
