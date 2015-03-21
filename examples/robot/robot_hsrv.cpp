
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>

#include <unistd.h>
#include <utils/log.h>
#include <qao/question.h>
#include <qao/candidate.h>
#include "robot_hsrv.h"

using namespace std;

robot_hsrv::~robot_hsrv() {
}

void robot_hsrv::read(int sock, short event, void* arg) {
	robot_hsrv *srv = (robot_hsrv *)arg;
	void *buffer;
	size_t len = 0;
	bool partition = false;
	
	/* 接收数据 */
	buffer = srv->ev_recv(len, partition);
	if ((int)len <= 0) {
		/* = 0: 客户端断开连接，在这里移除读事件并且释放客户数据结构 */
		/* < 0: 出现了其它的错误，在这里关闭socket，移除事件并且释放客户数据结构 */
		delete srv;
		return;
	} else if (partition) {
		return;
	}
	
	/* 检查数据是否有效 */
	if (buffer == NULL) {
		return;
	}
	
	/* 把序列化数据转换为Candidate对象 */
	try {
		candidate *qao = new candidate((const char*)buffer, len);
		
		/* 记录站点信息, 显示对象内容 */
#ifdef CFG_QAO_TRACE		
		qao->trace("robot_hsrv");
//		qao->dump_trace();
#endif
#ifdef CFG_QAO_DUMP
		qao->dump();
#endif
		
		/* 把Candidate发给Hub */
		if (!srv->ev_send(qao)) {
			delete qao;
		}
		
	} catch (const char *msg) {
		printf("Get error question");
	}
	   
	/* 释放缓冲区 */
	srv->recv_done(buffer);
}


void robot_hsrv::send_done(qao_base *qao, bool send_ok) {
	/* 释放发送对象 */
	delete qao;
}

