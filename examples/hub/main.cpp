#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <event.h>
#include <log.h>
#include <evsrv.h>
#include "hub_csrv.h"
#include "hub_ssrv.h"
#include "robot_clnt.h"

#define CFG_LISTEN_IP		"127.0.0.1"
#define CFG_LISTEN_PORT		10000

static unsigned int port = CFG_LISTEN_PORT;
static char ip_str[256] = CFG_LISTEN_IP;
static bool fork_to_background = false;

static struct option long_options[] = {
	{"help",  no_argument, 0,  'h' },
	{"port",    required_argument, 0,  'p' },
	{"ip",      required_argument, 0,  'i' },
	{"deamon",  no_argument, 0,  'd' },
	{0,         0,                 0,  0 }
};

static void help(void) {
	printf("Usage: lb-server\n");
	printf("	-h, --help       display this help and exit\n");
	printf("	-i, --ip         setting listen ip address\n");
	printf("	-p, --port       setting listen port\n");
	printf("	-d, --deamon     daemonize\n");
}

robot_clnt *robot_sock = NULL;

static bool parser_opt(int argc, char **argv) {
	int c;
	
	while (1) {
		int option_index = 0;
		
		c = getopt_long(argc, argv, "hp:i:d",
					long_options, &option_index);
		if (c == -1)
		    break;

		switch (c) {
		case 'p':
			port = atoi(optarg);
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


static void *thread_clb(void *args) {
	evsrv<hub_csrv> *srv;
	try {
		srv = new evsrv<hub_csrv>(ip_str, (unsigned short)port);
	} catch(const char *msg) {
		printf("Error out: %s\n", msg);
		exit(-1);
	}
	
	/* 设置服务Socket选项 */
	int yes = 1;
	if (srv->setskopt(SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
		printf("setsockopt error");
		exit(-1);
	}
	
	/* 服务监听 */
	if (!srv->loop()) {
		printf("hub starting clb server failed\n");
	}
	return NULL;
}


static void *thread_slb(void *args) {
	/* 创建和SLB通信的服务 */
	evsrv<hub_ssrv> *srv;
	try {
		srv = new evsrv<hub_ssrv>(ip_str, (unsigned short)(port + 2000));
	} catch(const char *msg) {
		printf("Error out: %s\n", msg);
		exit(-1);
	}
	
	/* 设置服务Socket选项 */
	int yes = 1;
	if (srv->setskopt(SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
		printf("setsockopt error");
		exit(-1);
	}
	
	/* 服务监听 */
	if (!srv->loop()) {
		printf("starting server failed\n");
	}
	return NULL;
}


static void *thread_robot(void *args) {
	/* 创建基于Event的客户端，用于发送命令和接受响应 */	
	evclnt<robot_clnt> clnt(ip_str, (unsigned short)(port + 1000));
	robot_sock = clnt.create_evsock();
	if (robot_sock) {
    	clnt.loop();
    }
    return NULL;
}


int main(int argc, char* argv[]) {
	/* 设置日志参数 */
	LOG_OPEN("hub");
	
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
	}
	
	/* 创建QAO对象管理器 */
	
		
    /* 创建和Robot通信的客户端 */
	pthread_t th_robot;
	if (pthread_create(&th_robot, NULL, thread_robot, NULL) != 0) {
		printf("Create worker thread failed");
		return -1;
	}
	
	/* 创建和CLB通信的服务 */
	pthread_t th_clb;
	if (pthread_create(&th_clb, NULL, thread_clb, NULL) != 0) {
		printf("Create worker thread failed");
		return -1;
	}
	
	/* 创建和SLB通信的服务 */
	pthread_t th_slb;
	if (pthread_create(&th_slb, NULL, thread_slb, NULL) != 0) {
		printf("Create worker thread failed");
		return -1;
	}
	
	/* 创建动态控制、配置服务 */
	while (1) {
		sleep(100);
	}
	return 0;
}

