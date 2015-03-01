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
#include "lbsrv.h"

#define CFG_LISTEN_IP		"127.0.0.1"
#define CFG_LISTEN_PORT		10000

struct sock_ev {
    struct event* read_ev;
    struct event* write_ev;
    char* buffer;
};

static struct event_base* base;

static unsigned int port = CFG_LISTEN_PORT;
static char ip_str[256] = CFG_LISTEN_IP;
static bool foreground = false;

static struct option long_options[] = {
    {"help",  no_argument, 0,  'h' },
    {"port",    required_argument, 0,  'p' },
    {"ip",      required_argument, 0,  'i' },
    {"foreground",    no_argument, 0,  'f' },
    {0,         0,                 0,  0 }
};

static void help(void) {
	printf("Usage: lb-server\n");
	printf("	-h, --help       display this help and exit\n");
	printf("	-i, --ip         setting listen ip address\n");
	printf("	-p, --port       setting listen port\n");
	printf("	-f, --foreground running in foreground\n");
}

static bool parser_opt(int argc, char **argv) {
    int c;
    int digit_optind = 0;
	
    while (1) {
        int this_option_optind = optind ? optind : 1;
        int option_index = 0;

        c = getopt_long(argc, argv, "hp:i:f",
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

        case 'f':
        	foreground = true;
            break;

        default:
            help();
            return false;
        }
    }
    return true;
}

int main(int argc, char* argv[]) {
    struct sockaddr_in sk_addr;
    int sock;
    int yes;
    
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
    
    /* 创建服务 */
    evsrv<lbsrv> *srv;
    try {
    	srv = new evsrv<lbsrv>(ip_str, (unsigned short)port);
    } catch(const char *msg) {
    	printf("Error out: %s\n", msg);
    	return -1;
    }
    
    /* 设置服务Socket选项 */
    yes = 1;
    if (srv->setskopt(SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
    	printf("setsockopt error");
    	return -1;
    }
    
    /* 服务监听 */
    if (!srv->loop()) {
    	printf("starting server failed\n");
    }
}

