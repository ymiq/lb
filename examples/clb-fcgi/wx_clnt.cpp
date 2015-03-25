
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include "wx_clnt.h"
#include <qao/answer.h>



void wx_clnt::send_done(void *buf, size_t len, bool send_ok) {
}


void wx_clnt::send_done(qao_base *qao, bool send_ok) {
	delete qao;
}

	
void wx_clnt::read(int sock, short event, void* arg) {
	wx_clnt *clnt = (wx_clnt *)arg;
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
	
	/* 检查数据是否有效 */
	if (buffer == NULL) {
		return;
	}
		
	/* 接收数据处理完成，释放资源 */
	clnt->recv_done(buffer);
}

