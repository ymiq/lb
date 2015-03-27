
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>

#include <unistd.h>
#include <utils/log.h>
#include <clb/clb_tbl.h>
#include <qao/cctl_req.h>
#include <qao/cctl_rep.h>
#include <qao/cdat_wx.h>
#include <stat/stat_man.h>
#include <clb/clb_grp.h>
#include "clb_srv.h"

using namespace std;
extern "C" {
	extern int clb_srv_group_id;
};

clb_srv::clb_srv(int fd, struct event_base* base) : evsock(fd, base) {
	plb = clb_tbl::get_inst();
	pgrp = clb_grp::get_inst();
	prcu = rcu_man::get_inst();
	tid = prcu->tid_get();
}


clb_srv::~clb_srv() {
}
	
void clb_srv::read(int sock, short event, void* arg) {
	clb_srv *srv = (clb_srv *)arg;
	void *buffer;
	size_t len = 0;
	bool partition;
	
	/* 接收数据 */
	buffer = srv->ev_recv(len, partition);
	if (partition) {
		return;
	} else if ((int)len <= 0) {
		/* = 0: 客户端断开连接，在这里移除读事件并且释放客户数据结构 */
		/* < 0: 出现了其它的错误，在这里关闭socket，移除事件并且释放客户数据结构 */
		srv_unbind(srv);
		delete srv;
		return;
	} 
	
	/* 检查数据是否有效 */
	if (buffer == NULL) {
		return;
	}
	
	/* 处理该段数据 */
	try {
		char *pxml = (char*)buffer + sizeof(serial_data);
	
			/* 获取xml数据长度 */
		int xml_len = strlen(pxml);
	
		/* Worker线程主处理开始 */
	    srv->prcu->job_start(srv->tid);
	    
		/* 把XML数据转换为对象 */
		cdat_wx wx((char*)pxml, xml_len);
		unsigned long hash = wx.hash;
#ifdef CFG_QAO_TRACE		
		wx.trace("clb_srv(%d)", clb_srv_group_id);
#endif

		/* 绑定Server和QAO */
		qao_srv_bind(&wx, srv);

	    /* 分发数据包 */
	    unsigned int lb_status = 0;
	    unsigned int stat_status = 0;
	    evclnt<clb_clnt> *pclnt = srv->plb->get_clnt(hash, lb_status, stat_status);
	    
    	/* 把数据包写入当前Socket */
	    if ((pclnt != NULL) && lb_status) {
	    	clb_clnt *clnt_sk = pclnt->get_evsock();
	    	if (clnt_sk) {
		    	size_t wlen;
		    	char *serial = (char*)wx.serialization(wlen);
		    	if (serial) {
		    		if(!clnt_sk->ev_send_inter_thread(serial, wlen)) {
		    			qao_srv_unbind(&wx);
		    			delete serial;
		    		}
		    	} else {
		    		qao_srv_unbind(&wx);
		    	}
		    } else {
		    	qao_srv_unbind(&wx);
		    }
	    } else {
		    qao_srv_unbind(&wx);
	    }
	    
	    /* 统计处理 */
	    if (stat_status) {	
//		    srv->pstat->stat(hash, 1);	    
	    }

		/* Worker线程主处理结束 */
	    srv->prcu->job_end(srv->tid);

	} catch(const char *msg) {
		LOGE("error: %s", msg);
	}
	
	/* 释放缓冲区 */
	srv->recv_done(buffer);
}


void clb_srv::send_done(void *buf, size_t len, bool send_ok) {
	delete[] (char*)buf;
}


void clb_srv::send_done(qao_base *qao, bool send_ok) {
	/* 销毁对象 */
	delete qao;
}

	