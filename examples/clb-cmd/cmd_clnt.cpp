﻿
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include "cmd_clnt.h"

bool cmd_clnt::open_timer(void) {
	/* 设置定时器 */
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	evtimer_set(&ev, timer_cb, this); 
	if (event_base_set(evbase, &ev) < 0) {
		LOGE("event_base_set error");
		return false;
	}
	if (event_add(&ev, &tv) < 0) {
		LOGE("event_base_set error");
		return false;
	}
	return true;
}


void cmd_clnt::send_done(void *buf, size_t len, bool send_ok) {
}


void cmd_clnt::send_done(qao_base *qao, bool send_ok) {
}


void cmd_clnt::read(int sock, short event, void* arg) {
	cmd_clnt *clnt = (cmd_clnt *)arg;
	void *buffer;
	size_t size;
	bool partition = false;
	
	/* 接收数据 */
	buffer = clnt->ev_recv(size, partition);
	if (partition) {
		return;
	} else if ((int)size <= 0) {
		/* = 0: 服务端断开连接，在这里移除读事件并且释放客户数据结构 */
		/* < 0: 出现了其它的错误，在这里关闭socket，移除事件并且释放客户数据结构 */
		exit(1);
	} 
	
	cctl_rep_factory rep((const char*)buffer, size);
	
	/* 简单退出!!! */
	if (!clnt->qao) {
		rep.dump();
		clnt->ev_close();
	} else {
		/* 清除屏幕内容, 并且把光标移到第一行第一列 */
		printf("\033[1H\033[2J");
		rep.dump();
	}
	
	/* 接收数据处理完成，释放资源 */
	clnt->recv_done(buffer);
}


void cmd_clnt::timer_cb(int sock, short event, void* arg) {
	cmd_clnt *clnt = (cmd_clnt *)arg;
	
	if (clnt->qao) {
		clnt->ev_send(clnt->qao);
	}
	
	clnt->open_timer();
}

