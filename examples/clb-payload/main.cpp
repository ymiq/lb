
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <evclnt.h>
#include "pl_clnt.h"
#include "http.h"

using namespace std;

#define CFG_CLBSRV_IP		"127.0.0.1"
#define CFG_CLBSRV_PORT		7000
#define CFG_SIM_THREADS		4

static unsigned int port = CFG_CLBSRV_PORT;
static char ip_str[256] = CFG_CLBSRV_IP;
static bool fork_to_background = false;
static bool http_payload = true;

static struct option long_options[] = {
    {"help",  no_argument, 0,  'h' },
    {"simple",  no_argument, 0,  's' },
    {"http",  no_argument, 0,  't' },
    {"deamon",  no_argument, 0,  'd' },
    {0,         0,                 0,  0 }
};

static void help(void) {
	printf("Usage: cmb-payload \n");
	printf("	-h, --help       display this help and exit\n");
	printf("	-i, --ip         setting listen ip address\n");
	printf("	-p, --port       setting listen port\n");
	printf("	-s, --simple     apply payload to long-link server\n");
	printf("	-t, --http       apply payload to http server\n");
	printf("	-d, --deamon     daemonize\n");
}


static bool parser_opt(int argc, char **argv) {
    int c;
	
    while (1) {
        int option_index = 0;

        c = getopt_long(argc, argv, "hstp:i:d",
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
		
        case 's':
        	http_payload = false;
            break;

        case 't':
        	http_payload = true;
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


static void rand_delay(int low, int high) {
	int distance = high - low;
	if (distance < 0) {
		distance = low;
	}
	int delay = (rand() % distance) + low;
	usleep(delay * 1000);
}


static void rand_company(char *name, int low, int high) {
	int distance = high - low;
	if (distance < 0) {
		distance = low;
	}
	int company = (rand() % distance) + low;
	sprintf(name, "www.%d.com", company);
}


static void rand_question(char *quest, int low, int high) {
	int distance = high - low;
	if (distance < 0) {
		distance = low;
	}
	int question = (rand() % distance) + low;
	sprintf(quest, "why %08d?", question);
}


static void rand_user(char *name, int low, int high) {
	int distance = high - low;
	if (distance < 0) {
		distance = low;
	}
	int user = (rand() % distance) + low;
	sprintf(name, "%d", user);
}


static const char format[] = {
	"<xml>"
	"<ToUserName><![CDATA[%s]]></ToUserName>"
	"<FromUserName><![CDATA[%s]]></FromUserName>"
	"<CreateTime>%u</CreateTime>"
	"<MsgType><![CDATA[text]]></MsgType>"
	"<Content><![CDATA[%s]]></Content>"
	"<MsgId>%lu</MsgId>"
	"</xml>"
};


static void rand_post(http *pclient, unsigned long msgid) {
	char company[128];
	char user[128];
	char question[128];
	char content[1024];
	struct timeval tv;
	
	gettimeofday(&tv, NULL);
	rand_company(company, 0, 16);
	rand_user(user, 0, 16);
	rand_question(question, 0, 16);
	snprintf(content, sizeof(content)-1, format, company, user, (unsigned int)tv.tv_sec, question, msgid);
	
	string url = "localhost/wxif/";
	string response;
	
	pclient->post(url, content, response);
}


static void rand_post(pl_clnt *sk, unsigned long msgid) {
	char company[128];
	char user[128];
	char question[128];
	struct timeval tv;
	
	gettimeofday(&tv, NULL);
	rand_company(company, 0, 16);
	rand_user(user, 0, 16);
	rand_question(question, 0, 16);
	
	char *content = new char[1024]();
	serial_data *pheader = (serial_data*)content;
	int datalen = snprintf((char*)(pheader->data), 1023 - sizeof(serial_data),
			 format, company, user, (unsigned int)tv.tv_sec, question, msgid);
	datalen += 1;
	pheader->length = datalen + sizeof(serial_data);
	pheader->datalen = datalen;
	sk->ev_send_inter_thread(pheader, pheader->length);
}


static unsigned long total_recv = 0;
static unsigned long total_sends = 0;
static struct timeval start_tv;

void dump_receive(void) {
	unsigned long reply = __sync_add_and_fetch(&total_recv, 1);
	if ((reply % 50000) == 0) {
		printf("REPLAY: %ld\n", reply);
	}
}

static void *pthread_ask_http(void *args) {
	http http_client;
	unsigned long msgid = ((unsigned long)pthread_self()) << 32;
	
	while (1) {
		
		/* 随机发送(100~1124)个包 */
		unsigned long send_packets = (rand() % 64) + 16;
		for (unsigned long cnt=0; cnt<=send_packets; cnt++) {

			string url = "localhost/wxif";
			string post = "<xml></xml>";
			string response;
			rand_post(&http_client, msgid++);
		}
		
		/* 随机Sleep一段时间(50~200ms) */
		rand_delay(1000, 2000);
		
		/* 显示问题数量和提问速度 */
		struct timeval tv;
		gettimeofday(&tv, NULL);
		unsigned long diff;
		if (tv.tv_usec >= start_tv.tv_usec) {
			diff = (tv.tv_sec - start_tv.tv_sec) * 10 + 
				(tv.tv_usec - start_tv.tv_usec) / 100000;
		} else {
			diff = (tv.tv_sec - start_tv.tv_sec - 1) * 10 + 
				(tv.tv_usec + 1000000 - start_tv.tv_usec) / 100000;
		}
		if (!diff) {
			diff = 10;
		}
		unsigned long questions = __sync_add_and_fetch(&total_sends, send_packets);
		printf("Questions: %ld, speed: %ld qps\n", questions, (questions * 10) / diff);
	}
	return NULL;
}


static void *pthread_ask_simple(void *args) {
	unsigned long msgid = ((unsigned long)pthread_self()) << 32;
	
	/* 创建客户端 */
	evclnt<pl_clnt> *pclnt = new evclnt<pl_clnt>("127.0.0.1", 7000);
	pl_clnt *sk = pclnt->create_evsock();
	if (sk) {
		/* 启动席位对应的线程 */
    	pclnt->loop_thread();
    }

	while (1) {
		
		/* 随机发送(100~1124)个包 */
		unsigned long send_packets = (rand() % 256) + 1024;
		for (unsigned long cnt=0; cnt<=send_packets; cnt++) {

			string url = "localhost/wxif";
			string post = "<xml></xml>";
			string response;
			if (sk) {
				rand_post(sk, msgid++);
			}
		}
		
		/* 随机Sleep一段时间(50~200ms) */
		rand_delay(200, 500);

		/* 显示问题数量和提问速度 */
		struct timeval tv;
		gettimeofday(&tv, NULL);
		unsigned long diff;
		if (tv.tv_usec >= start_tv.tv_usec) {
			diff = (tv.tv_sec - start_tv.tv_sec) * 10 + 
				(tv.tv_usec - start_tv.tv_usec) / 100000;
		} else {
			diff = (tv.tv_sec - start_tv.tv_sec - 1) * 10 + 
				(tv.tv_usec + 1000000 - start_tv.tv_usec) / 100000;
		}
		if (!diff) {
			diff = 10;
		}
		unsigned long questions = __sync_add_and_fetch(&total_sends, send_packets);
		printf("Questions: %ld, speed: %ld qps\n", questions, (questions * 10) / diff);
		
	}
	return NULL;
}


int main(int argc, char *argv[]) {
    /* 解析参数 */
    if (!parser_opt(argc, argv)) {
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
		LOG_OPEN("payload");
	} else {
		LOG_CONSOLE("payload");
	}

	/* 设置随机数种子 */
	srand((int)time(NULL));
	
	gettimeofday(&start_tv, NULL);
	
	/* 创建多线程，模拟用户提问 */
	pthread_t th_sim[CFG_SIM_THREADS];
	if (http_payload) {
		for (int i=0; i<CFG_SIM_THREADS; i++) {
			if (pthread_create(&th_sim[i], NULL, pthread_ask_http, NULL) != 0) {
				printf("Create thread failed\n");
				return -1;
			}
		}
	} else {
		for (int i=0; i<CFG_SIM_THREADS; i++) {
			if (pthread_create(&th_sim[i], NULL, pthread_ask_simple, NULL) != 0) {
				printf("Create thread failed\n");
				return -1;
			}
		}
	}
	
	/* 处理控制命令 */
	while (1) {
		sleep(1);
	}
	
	/* never reach here */	
	return 0;
}

