
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <qao/candidate.h>
#include "robot_clnt.h"
#include "hub_global.h"



void robot_clnt::send_done(void *buf, size_t len, bool send_ok) {
}


void robot_clnt::send_done(qao_base *qao, bool send_ok) {
	delete qao;
}


void robot_clnt::read(int sock, short event, void* arg) {
	robot_clnt *clnt = (robot_clnt *)arg;
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
	
	/* 由序列化数据转换为Question对象 */
	try {
		candidate *qao = new candidate((const char*)buffer, len);
		
		/* 记录站点信息, 显示对象内容 */
#ifdef CFG_QAO_TRACE		
		qao->trace("robot_clnt");
		qao->dump_trace();
#endif
#ifdef CFG_QAO_DUMP
		qao->dump();
#endif
		
		/* 根据目的HASH值，获取发送SLB socket */
		unsigned long hash = qao->hash;
		hub_ssrv *ssrv = ssrv_bind->get_val(hash);
		if (ssrv) {
			/* 把接收的对象转发给SLB客户端 */
			ssrv->ev_send(qao);
		}
		
	} catch (const char *msg) {
		printf("Get error question");
	}
	
	/* 接收数据处理完成，释放资源 */
	clnt->recv_done(buffer);
}

