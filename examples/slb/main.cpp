#include <string>
#include <sstream>
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
#include <utils/log.h>
#include <utils/hash_alg.h>
#include <qao/sclnt_decl.h>
#include <evclnt.h>
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

	/* 创建32个基于Event的应答席位 */
	for (int i=0; i<32; i++) {
		char nstr[256];
		sprintf(nstr, "www.%d.com", i+1);
		string name(nstr);
		evclnt<slb_clnt> *pclnt = new evclnt<slb_clnt>(ip_str, (unsigned short)(port));
		slb_clnt *sk = pclnt->create_evsock();
		if (sk) {
			sclnt_decl *qao = new sclnt_decl();
			sk->name = qao->name = name;
			sk->hash = qao->hash = company_hash(name.c_str());
			
			/* 发送当前席位ID */
			sk->ev_send(qao);
			
			/* 启动席位对应的线程 */
	    	pclnt->loop_thread();
	    }
	}
    
    while(1) {
    	sleep(100);
    }
    return 0;
}

