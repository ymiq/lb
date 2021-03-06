﻿
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>

#include <unistd.h>
#include <utils/log.h>
#include <qao/question.h>
#include <qao/answer.h>
#include <qao/ssrv_factory.h>
#include "hub_ssrv.h"
#include "hub_global.h"

using namespace std;


extern "C" {
	extern int hub_group_id;
}

hub_ssrv::~hub_ssrv() {
}

void hub_ssrv::read(int sock, short event, void* arg) {
	hub_ssrv *srv = (hub_ssrv *)arg;
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
	
	/* 由序列化数据还原成Answer对象 */
	try {
		ssrv_factory *qao = new ssrv_factory((const char*)buffer, len);
		if (qao->get_type() == QAO_SCLNT_DECL) {
			
			/* 绑定HASH 和 Socket  */
			sclnt_decl *decl = qao->get_sclnt_decl();
			unsigned long hash = decl->hash;
			
			ssrv_bind->add(hash, srv);
			
		} else {
		
		/* 记录站点信息, 显示对象内容 */
#ifdef CFG_QAO_TRACE	
			answer *aq = qao->get_answer();	
			aq->trace("hub_ssrv(%d)", hub_group_id);
//			qao->dump_trace();
#endif
#ifdef CFG_QAO_DUMP
			qao->dump();
#endif

			/* 把接收到Answer数据发送给fcgi */
			unsigned long token = qao->get_token();
			hub_csrv *csrv = csrv_bind->get_val(token);
			if (csrv != NULL) {
				if (!csrv->ev_send_inter_thread(qao)) {
					delete qao;
				}
			}
			csrv_bind->remove(token);
		}
		
	} catch (const char *msg) {
		printf("Get error question");
	}
	   
	/* 释放缓冲区 */
	srv->recv_done(buffer);
}


void hub_ssrv::send_done(qao_base *qao, bool send_ok) {
	/* 释放发送对象 */
	delete qao;
}

