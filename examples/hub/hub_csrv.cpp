
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>

#include <unistd.h>
#include <utils/log.h>
#include <qao/question.h>
#include "hub_global.h"
#include "hub_csrv.h"
#include "robot_clnt.h"


using namespace std;

hub_csrv::~hub_csrv() {
}

void hub_csrv::read(int sock, short event, void* arg) {
	hub_csrv *srv = (hub_csrv *)arg;
	void *buffer;
	size_t len = 0;
	bool partition = false;
	
	/* 接收数据 */
	buffer = srv->ev_recv(len, partition);
	if (partition) {
		return;
	} else if ((int)len <= 0) {
		/* = 0: 客户端断开连接，在这里移除读事件并且释放客户数据结构 */
		/* < 0: 出现了其它的错误，在这里关闭socket，移除事件并且释放客户数据结构 */
		delete srv;
		return;
	} 
	
	/* 检查数据是否有效 */
	if (buffer == NULL) {
		return;
	}
	
	/* 由序列化数据转换为Question对象 */
	try {
		question *q = new question((const char*)buffer, len);
		
		/* 绑定Server和QAO */
		csrv_bind->add(q->get_token(), srv);
		
		/* 记录站点信息, 显示对象内容 */
#ifdef CFG_QAO_TRACE		
		q->trace("hub_csrv");
//		q->dump_trace();
#endif
#ifdef CFG_QAO_DUMP
		q->dump();
#endif

		/* 把问题发给Robot */
		if (!robot_sock->ev_send_inter_thread(q)) {
			csrv_bind->remove(q->get_token());
			delete q;
		}
		
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

