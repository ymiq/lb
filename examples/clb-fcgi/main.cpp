
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>

#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <rcu_man.h>
#include <clb/clb_tbl.h>
#include <utils/log.h>
#include <fcgi_stdio.h>  
#include <evsrv.h>
#include <utils/hash_alg.h>
#include <qao/cdat_wx.h>
#include <stat/stat_man.h>
#include <stat/stat_tbl.h>
#include <stat/clb_stat.h>
#include "cmd_srv.h"
#include "cfg_db.h"
#include "wx_clnt.h"

using namespace std;
#define CFG_CMDSRV_IP		"127.0.0.1"
#define CFG_CMDSRV_PORT		8000

#define CFG_WORKER_THREADS	1
static wx_clnt *wx_sk = NULL;

static int lb_init(clb_tbl *plb, clb_grp *pgrp) {
	int ret = 0;
	
	cfg_db *db = new cfg_db();
	ret = db->init_lb_table(plb, pgrp);
	if (ret < 0) {
		LOGE("Can't create load balance talbe");
	}
	delete db;
	return ret; 
}


static int stat_init(stat_tbl_base *pstat) {
	/* 该函数要求在所有stat_table创建后再调用 */
	cfg_db *db = new cfg_db();
	int ret = db->init_stat_table(pstat);
	if (ret < 0) {
		LOGE("Can't create stat talbe");
	}
	delete db;
	return ret;  
}

// #define CFG_PERF_DUMP

#if defined(CFG_PERF_DUMP)
static unsigned long total_questions = 0;
static unsigned long recent_questions = 0;
static struct timeval start_tv;
static bool perf_init = false;
void dump_performance(void) {
	unsigned long diff;
	struct timeval tv;
	
	total_questions++;	
	recent_questions++;
	if (!perf_init) {
		gettimeofday(&start_tv, NULL);
		perf_init = true;
		return;
	}
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
		LOGI("REPLAY: %ld, speed: %ld qps\n", total_questions, (recent_questions * 100) / diff);
		start_tv = tv;
		recent_questions = 0;
	}
}
#endif


static void *thread_worker(void *args) {
    FCGX_Request request;
 
 	/* RCU相关初始化 */
	rcu_man *prcu = rcu_man::get_inst();
	int tid = prcu->tid_get();
	
	/* LB相关初始化 */
	clb_tbl *plb = clb_tbl::get_inst();
		
	/* 创建每线程统计表，并初始化 */
	stat_tbl<clb_stat, 1024> *pstat = new stat_tbl<clb_stat, 1024>;
	if (stat_init(pstat) < 0) {
		LOGE("statics table init failed");
		exit(1);
	}
	
	/* 连接处理 */
    FCGX_InitRequest(&request, 0, 0);
    while (1) {
		unsigned long hash;
		
		/* 接受请求。如果多线程，可能需要加锁 */
		int rc = FCGX_Accept_r(&request);
		if (rc < 0) {
			break;
		}
		
		/* 获取POST内容 */
 		char *request_method = FCGX_GetParam("REQUEST_METHOD", request.envp);
		if (strcmp(request_method, "POST")) {
		    FCGX_FPrintF(request.out,  
		        "Content-type: text/html\r\n"  
		        "\r\n"  
		        "<title>FastCGI Hello! ");  
	        FCGX_Finish_r(&request);
	        continue;
		}
		char *content_length = FCGX_GetParam("CONTENT_LENGTH", request.envp);
		if (content_length == NULL) {
	        FCGX_Finish_r(&request);
	        continue;
		}
		int len = strtol(content_length, NULL, 10);
		if (len == 0) {
	        FCGX_Finish_r(&request);
	        continue;
		}

#if defined(CFG_PERF_DUMP)
		/* 统计性能 */
		dump_performance();
#endif		
		/* 复制数据到缓冲区 */
		char *buffer = new char[len+1];
		FCGX_GetStr(buffer, len, request.in);
		buffer[len] = '\0';
//		LOGI("WX: %s", buffer);
		
		try {
			/* Worker线程主处理开始 */
		    prcu->job_start(tid);
		    
			/* 把XML数据转换为对象 */
			cdat_wx wx(buffer, len);
			hash = wx.hash;
#ifdef CFG_QAO_TRACE		
			wx.trace("fcgi");
//			wx.dump_trace();
#endif

		    /* 分发数据包 */
		    unsigned int lb_status = 0;
		    unsigned int stat_status = 0;
		    evclnt<clb_clnt> *pclnt = plb->get_clnt(hash, lb_status, stat_status);
		    
	    	/* 把数据包写入当前Socket */
		    if ((pclnt != NULL) && lb_status) {
		    	clb_clnt *clnt_sk = pclnt->get_evsock();
		    	if (clnt_sk) {
			    	size_t wlen;
			    	char *serial = (char*)wx.serialization(wlen);
			    	if (serial) {
			    		clnt_sk->ev_send_inter_thread(serial, wlen);
			    	}
			    }
		    }
		    
		    /* 统计处理 */
		    if (stat_status) {	
			    pstat->stat(hash, 1);	    
		    }

			/* Worker线程主处理结束 */
		    prcu->job_end(tid);

		} catch(const char *msg) {
			LOGE("error: %s", msg);
		}
		
		delete buffer;
				  
	    /* 应答 */
	    FCGX_FPrintF(request.out,  
	        "Content-type: text/html\r\n"  
	        "\r\n"  
	        "<title>FastCGI Hello! ");  
	    
        FCGX_Finish_r(&request);
	}
	return NULL;  
}


void answer_reply(qao_base *qao) {
	/* 把消息发送到WX服务器 */
	if (wx_sk) {
		if (!wx_sk->ev_send_inter_thread(qao)) {
			delete qao;
		}
	} else {
		delete qao;
	}
}


int main(int argc, char *argv[]) {
	/* 开启日志记录 */
	LOG_OPEN("clb-cfgi");
	
	/* 创建RCU管理线程 */
	if (!rcu_man::init()) {
		LOGE("Create rcu worker thread failed");
	}
	
	/* 获取负载均衡HASH表 */
	clb_tbl *plb = clb_tbl::get_inst();
	clb_grp *pgrp = clb_grp::get_inst();
	
	/* 根据数据库信息创建负载均衡HASH表 */
	if (lb_init(plb, pgrp) < 0) {
		LOGE("Can't create hash table");
		return -1;
	}
	
	/* FastCGI相关初始化 */
	FCGX_Init();
	
	/* 创建工作线程 */
	pthread_t th_worker[CFG_WORKER_THREADS];
	for (int idx=0; idx<CFG_WORKER_THREADS; idx++) {
		if (pthread_create(&th_worker[idx], NULL, thread_worker, NULL) != 0) {
			LOGE("Create worker thread failed");
			return -1;
		}
	}
	
	
	/* 创建动态配置命令服务 */
	try {
		/* 创建微信客户端 */
		evclnt<wx_clnt> pclnt(CFG_CMDSRV_IP, 5000);
		wx_sk = pclnt.evconnect();
		if (wx_sk) {
			/* 启动线程处理数据发送 */
	    	pclnt.loop_thread();
	    }
	
		evsrv<cmd_srv> srv(CFG_CMDSRV_IP, CFG_CMDSRV_PORT);
	
	    /* 设置服务Socket选项 */
	    int yes = 1;
	    if (srv.setskopt(SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
	    	LOGE("setsockopt error");
	    	return -1;
	    }
	
	    /* 服务监听 */
	    if (!srv.loop()) {
	    	LOGE("starting server failed");
	    }
	    
	}catch(const char *msg) {
		LOGE("Create server failed");
		return -1;
	}

	/* never reach here */	
	return 0;
}

