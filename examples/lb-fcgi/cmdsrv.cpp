
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>

#include <unistd.h>
#include <log.h>
#include "cmdsrv.h"

using namespace std;

cmdsrv::~cmdsrv() {
}

void cmdsrv::read(int sock, short event, void* arg) {
	cmdsrv *srv = (cmdsrv *)arg;
	void *buffer;
	size_t size;
	
	/* 接收数据 */
	buffer = srv->ev_recv(&size);
	if (size <= 0) {
		/* = 0: 客户端断开连接，在这里移除读事件并且释放客户数据结构 */
		/* < 0: 出现了其它的错误，在这里关闭socket，移除事件并且释放客户数据结构 */
		delete srv;
	}
	
	/* 处理命令 */
	
	/* 释放缓冲区 */
	srv->ev_recv_done(buffer);
}

void cmdsrv::ev_send_done(EV_SEND *send) {
	/* 释放发送缓冲区 */
	free(send->buf);
}
