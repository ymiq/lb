
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <evclnt.h>
#include <clb_clnt.h>
#include <qao/answer.h>


void clb_clnt::send_done(void *buf, size_t len, bool send_ok) {
	delete[] (char*)buf;
}


void clb_clnt::send_done(qao_base *qao, bool send_ok) {
	delete qao;
}


void clb_clnt::read(int sock, short event, void* arg) {
	clb_clnt *clnt = (clb_clnt *)arg;
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
	
	/* 处理数据 */
	answer *qao = new answer((const char*)buffer, len);
		
	/* 记录站点信息, 显示对象内容 */
#ifdef CFG_QAO_TRACE		
	qao->trace("clb_clnt");
	qao->dump_trace();
#endif
	delete qao;

	
		
	/* 接收数据处理完成，释放资源 */
	clnt->recv_done(buffer);
}

