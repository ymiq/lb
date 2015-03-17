
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>

#include <unistd.h>
#include <utils/log.h>
#include <qao/question.h>
#include <qao/answer.h>
#include "hub_ssrv.h"
#include "hub_global.h"

using namespace std;

hub_ssrv::~hub_ssrv() {
}

void hub_ssrv::read(int sock, short event, void* arg) {
	hub_ssrv *srv = (hub_ssrv *)arg;
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
	
	/* 由序列化数据还原成Answer对象 */
	try {
		answer *qao = new answer((const char*)buffer, len);
		
		/* 显示对象内容 */
		qao->dump();
		
		/* 把接收到Answer数据发送给fcgi */
		hub_csrv *csrv = csrv_bind->get_val(qao->get_token());
		if (csrv != NULL) {
			csrv->ev_send(qao);
		}
		
	} catch (const char *msg) {
		printf("Get error question");
	}
	   
	/* 释放缓冲区 */
	srv->recv_done(buffer);
}


void hub_ssrv::send_done(qao_base *qao, bool send_ok) {
	/* 释放发送对象 */
	if (!qao->dereference()) {
		delete qao;
	}
}

