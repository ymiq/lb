
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>

#include <unistd.h>
#include <log.h>
#include <qao/question.h>
#include "hub_csrv.h"

using namespace std;

hub_csrv::~hub_csrv() {
}

void hub_csrv::read(int sock, short event, void* arg) {
	hub_csrv *srv = (hub_csrv *)arg;
	void *buffer;
	size_t len = 0;
	bool fragment = false;
	
	/* 接收数据 */
	buffer = srv->ev_recv(len, fragment);
	if ((int)len <= 0) {
		/* = 0: 客户端断开连接，在这里移除读事件并且释放客户数据结构 */
		/* < 0: 出现了其它的错误，在这里关闭socket，移除事件并且释放客户数据结构 */
		delete srv;
		return;
	} else if (fragment) {
		return;
	}
	
	/* 由序列化数据还原对象 */
	try {
		question q((const char*)buffer, len);
		
		/* 显示对象内容 */
		q.dump();
	} catch (const char *msg) {
		printf("Get error question");
	}
	   
	/* 释放缓冲区 */
	srv->recv_done(buffer);
}


void hub_csrv::send_done(void *buf, size_t len, bool send_ok) {
	/* 释放发送缓冲区 */
//	free(send->buf);
}

