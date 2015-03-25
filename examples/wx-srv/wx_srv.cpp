
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>

#include <unistd.h>
#include <utils/log.h>
#include <qao/answer.h>
#include "wx_srv.h"

using namespace std;

wx_srv::wx_srv(int fd, struct event_base* base) : evsock(fd, base) {
	recv_questions = 0;
	gettimeofday(&start_tv, NULL);
}


wx_srv::~wx_srv() {
}


void wx_srv::dump_performance(unsigned long reply) {
	if ((reply % 100) == 0) {
		struct timeval tv;
		
		gettimeofday(&tv, NULL);
		unsigned long diff;
		if (tv.tv_usec >= start_tv.tv_usec) {
			diff = (tv.tv_sec - start_tv.tv_sec) * 10 + 
				(tv.tv_usec - start_tv.tv_usec) / 100000;
		} else {
			diff = (tv.tv_sec - start_tv.tv_sec - 1) * 10 + 
				(tv.tv_usec + 1000000 - start_tv.tv_usec) / 100000;
		}
		if (!diff) {
			diff = 10;
		}

		printf("REPLAY: %ld, speed: %ld qps\n", reply, (reply * 10) / diff);
	}
}


void wx_srv::read(int sock, short event, void* arg) {
	wx_srv *srv = (wx_srv *)arg;
	void *buffer;
	size_t len;
	bool partition = false;
	
	/* 接收数据 */
	buffer = srv->ev_recv(len, partition);
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
		
		/* 增加计数器 */
		srv->recv_questions++;
		
		/* 记录站点信息, 显示对象内容 */
#ifdef CFG_QAO_TRACE		
		qao->trace("wx_srv");
#endif
#ifdef CFG_QAO_DUMP
		qao->dump();
#endif
		
		/* 把Candidate发给Hub */
		if (question_trace) {
			qao->dump_trace();
		} else {
			srv->dump_performance(srv->recv_questions);
		}
		delete qao;
		
	} catch (const char *msg) {
		printf("Get error question");
	}	
	
	/* 接收数据处理完成，释放资源 */
	srv->recv_done(buffer);
}


void wx_srv::send_done(void *buf, size_t len, bool send_ok) {
}


void wx_srv::send_done(qao_base *qao, bool send_ok) {
}

	