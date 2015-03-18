
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include "slb_clnt.h"
#include <qao/answer.h>



void slb_clnt::send_done(void *buf, size_t len, bool send_ok) {
}


void slb_clnt::send_done(qao_base *qao, bool send_ok) {
	delete qao;
}


void slb_clnt::read(int sock, short event, void* arg) {
	slb_clnt *clnt = (slb_clnt *)arg;
	void *buffer;
	size_t len;
	bool fragment = false;
	
	/* 接收数据 */
	buffer = clnt->ev_recv(len, fragment);
	if ((int)len <= 0) {
		/* = 0: 服务端断开连接，在这里移除读事件并且释放客户数据结构 */
		/* < 0: 出现了其它的错误，在这里关闭socket，移除事件并且释放客户数据结构 */
		exit(1);
	} else if (fragment) {
		return;
	}
	
	/* 把序列化数据转换为answer对象 */
	try {
		answer *qao = new answer((const char*)buffer, len);
		
		/* 记录站点信息, 显示对象内容 */
#ifdef CFG_QAO_TRACE		
		qao->trace("slb_clnt(%lx,%s)", clnt->hash, clnt->name.c_str());
		qao->dump_trace();
#endif
#ifdef CFG_QAO_DUMP
		qao->dump();
#endif
		
		/* 把Candidate发给Hub */
		clnt->ev_send(qao);
		
	} catch (const char *msg) {
		printf("Get error question");
	}	
	
	/* 接收数据处理完成，释放资源 */
	clnt->recv_done(buffer);
}

