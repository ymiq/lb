
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>

#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcgi_stdio.h>  
#include <rcu_man.h>
#include <lb_table.h>
#include <log.h>
#include <lbdb.h>

using namespace std;

#define CFG_WORKER_THREADS	1

static int lb_init(void) {
	int ret;
	
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


static void *thread_worker(void *args) {
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
	
	/* FastCGI相关初始化 */
	FCGX_Init();
	FCGX_Request request;  
	FCGX_InitRequest(&request, 0, 0);  
	
	/* 连接处理 */
	while (1) { 
		unsigned long int hash;
		
		/* 接收连接 */
	    int rc = FCGX_Accept_r(&request);  
	    if (rc < 0) {
	        break;
	    }
	    
	    /* 获取请求数据 */
	    
	    hash = 0x123456789UL + rand();
	    
		/* Worker线程主处理 */
	    prcu->job_start(tid);
	    
	    pstat->stat(hash);
	    
	    prcu->job_end(tid);

	    /* 应答 */
	    FCGX_FPrintF(request.out,  
	        "Content-type: text/html\r\n"  
	        "\r\n"  
	        "<title>FastCGI Hello! ");  
	    
	    /* 连接清理 */   
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
		sleep(1);
	}
	
	/* never reach here */	
	return 0;
}

