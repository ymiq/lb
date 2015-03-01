
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>

#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <rcu_man.h>
#include <lb_table.h>
#include <log.h>
#include <lbdb.h>
#include <fcgi_stdio.h>  
#include <openssl/md5.h>
#include <evsrv.h>
#include "cmdsrv.h"

using namespace std;

#define CFG_WORKER_THREADS	1

static int lb_init(void) {
	int ret = 0;
	
	lbdb *db = new lbdb();
	ret = db->lb_init();
	if (ret < 0) {
		LOGE("Can't create load balance talbe");
	}
	delete db;
	return ret; 
}


static int stat_init(stat_table *pstat) {
	/* 该函数要求在所有stat_table创建后再调用 */
	lbdb *db = new lbdb();
	int ret = db->stat_init(pstat);
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


static unsigned long int company_hash(const char *company) {
	MD5_CTX ctx;
	unsigned char md5[16];
	unsigned long int hash = 0;
	
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
	lb_table *plb = lb_table::get_inst();
		
	/* 创建每线程统计表，并初始化 */
	stat_table *pstat = new stat_table;
	if (stat_init(pstat) < 0) {
		LOGE("statics table init failed");
		exit(1);
	}
	
	/* 连接处理 */
    FCGX_InitRequest(&request, 0, 0);
    while (1) {
		unsigned long int hash;
		
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
		LOGD("C: %s,	U: %s,	Q: %s,	H: %lx", company, user, question, hash);
				  
		/* Worker线程主处理开始 */
	    prcu->job_start(tid);
	    
	    /* 分发数据包 */
	    bool running = false;
	    int handle = plb->get_handle(hash, &running);
	    if ((handle > 0) && running) {
	    	/* 把数据包写入当前Socket */
	    	write(handle, buffer, len);
	    	
		    /* 统计处理 */
		    pstat->stat(hash);	    
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
	LOG_OPEN("lb-cfgi");
	
	/* 创建RCU管理线程 */
	pthread_t th_rcu;
	if (pthread_create(&th_rcu, NULL, thread_rcu_man, NULL) < 0) {
		LOGE("Create rcu manage thread failed");
		return -1;
	}
	
	/* 根据数据库信息创建负载均衡HASH表 */
	if (lb_init() < 0) {
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

	/* 等待动态配置命令 */
	while (1) {
		evsrv<cmdsrv> srv("127.0.0.1", 8000);
		
		srv.loop();
	}
	
	/* never reach here */	
	return 0;
}

