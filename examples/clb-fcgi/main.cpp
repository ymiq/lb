
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>

#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <rcu_man.h>
#include <clb_tbl.h>
#include <log.h>
#include <fcgi_stdio.h>  
#include <openssl/md5.h>
#include <evsrv.h>
#include "cmd_srv.h"
#include "cfg_db.h"

using namespace std;
#define CFG_CMDSRV_IP		"127.0.0.1"
#define CFG_CMDSRV_PORT		8000

#define CFG_WORKER_THREADS	1

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


static int stat_init(stat_tbl *pstat) {
	/* 该函数要求在所有stat_table创建后再调用 */
	cfg_db *db = new cfg_db();
	int ret = db->init_stat_table(pstat);
	if (ret < 0) {
		LOGE("Can't create stat talbe");
	}
	delete db;
	return ret;  
}


static void *thread_rcu_man(void *args) {
	
	rcu_man *prcu = rcu_man::get_inst();
	while (1) {
		/* 循环调用RCU，释放所有进入Grace Period的对象或者缓冲区 */
		prcu->job_free();
		
		/* 延时 */
		usleep(100*1000);
	}
	return NULL;
}


static bool content_parser(char *content, char *company, char *user, char *question) {
	char *pstart;
	char *pend;
	
	struct match {
		const char *stoken;
		const char *etoken;
		char *data;
	} xml_match [] = 
	{
		{"<company>", "</company>", company},
		{"<user>", "</user>", user},
		{"<question>", "</question>", question}
	};
	
	pstart = strstr(content, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	if (pstart == NULL) {
		return false;
	}
	
	for (int i=0; i<3; i++) {
		char *dst = xml_match[i].data;
		int max = 127;
		pstart = strstr(content, xml_match[i].stoken);
		if (pstart == NULL) {
			return false;
		}
		pstart += strlen(xml_match[i].stoken);
		pend = strstr(content, xml_match[i].etoken);
		if (pend == NULL) {
			return false;
		}
		if (pend == pstart) {
			return false;
		}
		while ((pstart != pend) && max--) {
			*dst++ = *pstart++;
		}
		*dst = '\0';
	}
	return true;	
}


static unsigned long company_hash(const char *company) {
	MD5_CTX ctx;
	unsigned char md5[16];
	unsigned long hash = 0;
	
	MD5_Init(&ctx);
	MD5_Update(&ctx, company, strlen(company));
	MD5_Final(md5, &ctx);
	
	memcpy(&hash, md5, sizeof(hash));
	
	return hash;
}


static void *thread_worker(void *args) {
    FCGX_Request request;
	char buffer[4096];
	char company[128];
	char user[128];
	char question[128];
 
 	/* RCU相关初始化 */
	rcu_man *prcu = rcu_man::get_inst();
	int tid = prcu->tid_get();
	
	/* LB相关初始化 */
	clb_tbl *plb = clb_tbl::get_inst();
		
	/* 创建每线程统计表，并初始化 */
	stat_tbl *pstat = new stat_tbl;
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
		len = (len > 4000) ? 4000 : len;
		FCGX_GetStr(buffer, 4000, request.in);
		buffer[len] = '\0';
				
		/* 解析请求内容 */
		if (content_parser(buffer, company, user, question) == false) {
	        FCGX_Finish_r(&request);
	        continue;
		}
		hash = company_hash(company);
		// LOGD("C: %s,	U: %s,	Q: %s,	H: %lx", company, user, question, hash);
				  
		/* Worker线程主处理开始 */
	    prcu->job_start(tid);
	    
	    /* 分发数据包 */
	    unsigned int lb_status = 0;
	    unsigned int stat_status = 0;
	    int handle = plb->lb_handle(hash, lb_status, stat_status);
    	/* 把数据包写入当前Socket */
	    if ((handle > 0) && lb_status) {
	    	write(handle, buffer, len);
	    }
	    /* 统计处理 */
	    if (stat_status) {	
		    pstat->stat(hash, 1);	    
	    }
	    	    
		/* Worker线程主处理结束 */
	    prcu->job_end(tid);

	    /* 应答 */
	    FCGX_FPrintF(request.out,  
	        "Content-type: text/html\r\n"  
	        "\r\n"  
	        "<title>FastCGI Hello! ");  
	    
        FCGX_Finish_r(&request);
	}
	return NULL;  
}


int main(int argc, char *argv[]) {
	/* 开启日志记录 */
	LOG_OPEN("clb-cfgi");
	
	/* 创建RCU管理线程 */
	pthread_t th_rcu;
	if (pthread_create(&th_rcu, NULL, thread_rcu_man, NULL) < 0) {
		LOGE("Create rcu manage thread failed");
		return -1;
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

