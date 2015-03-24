
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
	recent_questions = 0;
	gettimeofday(&start_tv, NULL);
}


wx_srv::~wx_srv() {
}

void wx_srv::dump_performance(unsigned long reply) {
	unsigned long diff;
	struct timeval tv;
		
	recent_questions++;
	gettimeofday(&tv, NULL);
	if (tv.tv_usec >= start_tv.tv_usec) {
		diff = (tv.tv_sec - start_tv.tv_sec) * 100 + 
			(tv.tv_usec - start_tv.tv_usec) / 10000;
	} else {
		diff = (tv.tv_sec - start_tv.tv_sec - 1) * 100 + 
			(tv.tv_usec + 1000000 - start_tv.tv_usec) / 10000;
	}
	if (!diff) {
		diff = 10;
	}
	if (diff >= 100) {
		printf("REPLAY: %ld, speed: %ld qps\n", reply, (recent_questions * 100) / diff);
		start_tv = tv;
		recent_questions = 0;
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
		delete srv;
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

	