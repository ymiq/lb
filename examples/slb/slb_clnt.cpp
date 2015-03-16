
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include "slb_clnt.h"



void slb_clnt::send_done(void *buf, size_t len, bool send_ok) {
}


void slb_clnt::send_done(qao_base *qao, bool send_ok) {
}


void slb_clnt::read(int sock, short event, void* arg) {
	slb_clnt *clnt = (slb_clnt *)arg;
	void *buffer;
	size_t size;
	bool fragment = false;
	
	/* 接收数据 */
	buffer = clnt->ev_recv(size, fragment);
	if ((int)size <= 0) {
		/* = 0: 服务端断开连接，在这里移除读事件并且释放客户数据结构 */
		/* < 0: 出现了其它的错误，在这里关闭socket，移除事件并且释放客户数据结构 */
		exit(1);
	} else if (fragment) {
		return;
	}
	
	
	/* 接收数据处理完成，释放资源 */
	clnt->recv_done(buffer);
}

