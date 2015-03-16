﻿
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>

#include <unistd.h>
#include <utils/log.h>
#include <qao/question.h>
#include "hub_csrv.h"
#include "robot_clnt.h"

extern "C" {
extern robot_clnt *robot_sock;
};

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
	
	/* 由序列化数据转换为Question对象 */
	try {
		question *q = new question((const char*)buffer, len);
		
		/* 显示对象内容 */
		q->dump();
		
		/* 把问题发给Robot */
		robot_sock->ev_send(q);
		
	} catch (const char *msg) {
		printf("Get error question");
	}
	   
	/* 释放缓冲区 */
	srv->recv_done(buffer);
}


void hub_csrv::send_done(qao_base *qao, bool send_ok) {
	/* 释放发送对象 */
	delete qao;
}

