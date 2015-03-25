
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include "pl_clnt.h"
#include <qao/answer.h>

extern "C" {
	extern bool trace_show;
};

void pl_clnt::send_done(void *buf, size_t len, bool send_ok) {
	delete[] (char*)buf;
}


void pl_clnt::send_done(qao_base *qao, bool send_ok) {
	delete qao;
}


void pl_clnt::read(int sock, short event, void* arg) {
	pl_clnt *clnt = (pl_clnt *)arg;
	void *buffer;
	size_t len;
	bool partition = false;
	
	/* 接收数据 */
	buffer = clnt->ev_recv(len, partition);
	if (partition) {
		return;
	} else if ((int)len <= 0) {
		/* = 0: 服务端断开连接，在这里移除读事件并且释放客户数据结构 */
		/* < 0: 出现了其它的错误，在这里关闭socket，移除事件并且释放客户数据结构 */
		exit(1);
	}
	
	/* 无效数据 */
	if (buffer == NULL) {
		return;
	}
	
	/* 把序列化数据转换为answer对象 */
	try {
		answer *qao = new answer((const char*)buffer, len);
		
		/* 记录站点信息, 显示对象内容 */
#ifdef CFG_QAO_TRACE		
		qao->trace("pl_clnt");
		if (trace_show) {
			qao->dump_trace();
		}
#endif
#ifdef CFG_QAO_DUMP
		qao->dump();
#endif
		
		/* 把Candidate发给Hub */
		dump_receive();
		delete qao;
		
	} catch (const char *msg) {
		printf("Get error question");
	}	
	
	/* 接收数据处理完成，释放资源 */
	clnt->recv_done(buffer);
}

