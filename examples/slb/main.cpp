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
#include "slb_clnt.h"

#define CFG_LISTEN_IP		"127.0.0.1"
#define CFG_LISTEN_PORT		12000

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
	printf("Usage: robot server\n");
	printf("	-h, --help       display this help and exit\n");
	printf("	-i, --ip         setting listen ip address\n");
	printf("	-p, --port       setting listen port\n");
	printf("	-d, --deamon     daemonize\n");
}

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

int main(int argc, char* argv[]) {
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

	/* 创建基于Event的客户端，用于发送命令和接受响应 */	
	evclnt<slb_clnt> clnt(ip_str, (unsigned short)(port + 1000));
	slb_clnt *sk = clnt.create_evsock();
	if (sk) {
    	clnt.loop();
    }
    return 0;
}

