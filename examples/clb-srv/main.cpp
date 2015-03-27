
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
#include <getopt.h>
#include <utils/hash_alg.h>
#include <qao/cdat_wx.h>
#include <stat/stat_man.h>
#include <stat/stat_tbl.h>
#include <stat/clb_stat.h>
#include <cmd_srv.h>
#include <cfg_db.h>
#include <hash_bind.h>
#include "clb_srv.h"

using namespace std;
#define CFG_CLBSRV_IP			"127.0.0.1"
#define CFG_CLBSRV_PORT_BASE	10000

#define CFG_WORKER_THREADS	1

#define QAO_BIND		hash_bind<clb_srv*, 1024, 16>
QAO_BIND *qao_bind;

static unsigned int port = CFG_CLBSRV_PORT_BASE;
static char ip_str[256] = CFG_CLBSRV_IP;
static bool fork_to_background = false;
int clb_srv_group_id = 0;

static struct option long_options[] = {
    {"help",  no_argument, 0,  'h' },
    {"group",    required_argument, 0,  'g' },
    {"ip",      required_argument, 0,  'i' },
    {"deamon",  no_argument, 0,  'd' },
    {0,         0,                 0,  0 }
};

static void help(void) {
	printf("Usage: robot server\n");
	printf("	-h, --help       display this help and exit\n");
	printf("	-i, --ip         setting listen ip address\n");
	printf("	-g, --group      setting listen group\n");
	printf("	-d, --deamon     daemonize\n");
}

static bool parser_opt(int argc, char **argv) {
    int c;
	
    while (1) {
        int option_index = 0;

        c = getopt_long(argc, argv, "hg:i:d",
                 long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case 'g':
        	clb_srv_group_id = atoi(optarg);
        	port += clb_srv_group_id * 10;
            break;

        case 'i':
        	strncpy(ip_str, optarg, 255);
            break;

        case 'd':
        	fork_to_background = true;
            break;

        default:
            help();
            return false;
        }
    }
    return true;
}


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


#if CFG_CLBSRV_MULTI_THREAD
static void *thread_worker(void *args) {
	/* 创建每线程统计表，并初始化 */
	stat_tbl<clb_stat, 1024> *pstat = new stat_tbl<clb_stat, 1024>;
	if (stat_init(pstat) < 0) {
		LOGE("statics table init failed");
		exit(1);
	}
	
	evsrv<clb_srv> srv(ip_str, (unsigned short)(port+1000));

    /* 设置服务Socket选项 */
    int yes = 1;
    if (srv.setskopt(SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
    	LOGE("setsockopt error");
    	return NULL;
    }

    /* 建立服务 */
    if (!srv.loop()) {
    	LOGE("starting server failed");
    }

	return NULL;  
}
#endif


void answer_reply(qao_base *qao) {
	unsigned long token = qao->get_token();
	clb_srv *srv = qao_bind->get_val(token);
	if (srv != NULL) {
#if CFG_CLBSRV_MULTI_THREAD
		if (!srv->ev_send_inter_thread(qao)) {
			delete qao;
		}
#else
		if (!srv->ev_send(qao)) {
			delete qao;
		}
#endif
		qao_bind->remove(token);
	} else {
		delete qao;
	}
}


void qao_srv_bind(qao_base *qao, clb_srv *srv) {
	unsigned long token = qao->get_token();
	qao_bind->add(token, srv);
}


void srv_unbind(clb_srv *srv) {
	qao_bind->remove(srv);
}

void qao_srv_unbind(qao_base *qao) {
	unsigned long token = qao->get_token();
	qao_bind->remove(token);
}


int main(int argc, char *argv[]) {
    /* 解析参数 */
    if (!parser_opt(argc, argv)) {
    	return 0;
    }
    if (!port || (port <= 1024) || (port >= 0x10000)) {
    	printf("Invalid port number\n");
    	return 0;
    }
    
	if (fork_to_background) {
		int pid = fork();
		switch (pid) {
		case -1:
		    LOGE("can't fork() to background");
		    break;
		case 0:
		    /* child process */
		    break;
		default:
		    /* parent process */
		    LOGI("forked to background, pid is %d", pid);
		    return 0;
		}
		
		/* 开启日志记录 */
		LOG_OPEN("clb-srv");
	} else {
		LOG_CONSOLE("clb-srv");
	}
	
	/* 创建RCU管理线程 */
	if (!rcu_man::init()) {
		LOGE("Create rcu worker thread failed");
	}
	
	/* 获取负载均衡HASH表 */
	clb_tbl *plb = clb_tbl::get_inst();
	clb_grp *pgrp = clb_grp::get_inst();
		
#if !CFG_CLBSRV_MULTI_THREAD
	/* 创建event base */
	struct event_base* base = event_base_new();
	pgrp->set_event_base(base);
#endif
	
	/* 根据数据库信息创建负载均衡HASH表 */
	if (lb_init(plb, pgrp) < 0) {
		LOGE("Can't create hash table");
		return -1;
	}
	
	try {
		qao_bind = new QAO_BIND();
	} catch (const char *msg) {
		LOGE("Can't creat qao_bind: %s", msg);
		exit(-1);
	}

	/* 创建工作线程 */
#if CFG_CLBSRV_MULTI_THREAD
	pthread_t th_worker[CFG_WORKER_THREADS];
	for (int idx=0; idx<CFG_WORKER_THREADS; idx++) {
		if (pthread_create(&th_worker[idx], NULL, thread_worker, NULL) != 0) {
			LOGE("Create worker thread failed");
			return -1;
		}
	}
#else		
	/* 创建每线程统计表，并初始化 */
	stat_tbl<clb_stat, 1024> *pstat = new stat_tbl<clb_stat, 1024>;
	if (stat_init(pstat) < 0) {
		LOGE("statics table init failed");
		exit(1);
	}
	
	evsrv<clb_srv> srv(ip_str, (unsigned short)(port+1000), base);

    /* 设置服务Socket选项 */
    int yes = 1;
    if (srv.setskopt(SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
    	LOGE("setsockopt error");
    	return -1;
    }

    /* 建立服务 */
    if (!srv.evlisten()) {
    	LOGE("starting server failed");
    }

#endif

	/* 创建动态配置命令服务 */
	try {
#if CFG_CLBSRV_MULTI_THREAD
		evsrv<cmd_srv> srv(CFG_CLBSRV_IP, (unsigned short)port);
#else
		evsrv<cmd_srv> srv(CFG_CLBSRV_IP, (unsigned short)port, base);
#endif	
	    /* 设置服务Socket选项 */
	    int yes = 1;
	    if (srv.setskopt(SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
	    	LOGE("setsockopt error");
	    	return -1;
	    }
	
#if CFG_CLBSRV_MULTI_THREAD
	    /* 服务监听 */
	    if (!srv.loop()) {
	    	LOGE("starting server failed");
	    }
#else
	    /* 服务监听 */
	    if (!srv.evlisten()) {
	    	LOGE("starting server failed");
	    }
	    
		/* 循环事件处理 */
		event_base_dispatch(base);
#endif
	    
	}catch(const char *msg) {
		LOGE("Create server failed");
		return -1;
	}
	
	/* never reach here */	
	return 0;
}

