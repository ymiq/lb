﻿#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <event.h>
#include <utils/log.h>
#include <evsrv.h>
#include "robot_hsrv.h"

#define CFG_LISTEN_IP			"127.0.0.1"
#define CFG_LISTEN_PORT_BASE	40000

static unsigned int port = CFG_LISTEN_PORT_BASE;
static char ip_str[256] = CFG_LISTEN_IP;
static bool fork_to_background = false;

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
	printf("	-p, --group      setting listen group\n");
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
        	port += 100 * atoi(optarg);
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

int main(int argc, char* argv[]) {
    /* 设置日志参数 */
    LOG_OPEN("robot");
    
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

    /* 创建服务 */
    evsrv<robot_hsrv> *srv;
    try {
    	srv = new evsrv<robot_hsrv>(ip_str, (unsigned short)(port + 1000));
    } catch(const char *msg) {
    	printf("Error out: %s\n", msg);
    	return -1;
    }
    
    /* 设置服务Socket选项 */
    int yes = 1;
    if (srv->setskopt(SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
    	printf("setsockopt error");
    	return -1;
    }
    
    /* 服务监听 */
    if (!srv->loop()) {
    	printf("starting server failed\n");
    }
}

