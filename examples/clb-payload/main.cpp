﻿
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

#define CFG_CLBSRV_IP			"127.0.0.1"
#define CFG_CLBSRV_PORT_BASE	10000
#define CFG_SIM_THREADS			4

static unsigned int port = CFG_CLBSRV_PORT_BASE;
static char ip_str[256] = CFG_CLBSRV_IP;
static bool fork_to_background = false;
static bool http_payload = true;
static int speed = 0;
bool trace_show = false;

static struct option long_options[] = {
    {"help",  no_argument, 0,  'h' },
    {"long",  no_argument, 0,  'l' },
    {"http",  no_argument, 0,  't' },
    {"speed",  required_argument, 0,  's' },
    {"trace",  no_argument, 0,  't' },
    {"deamon",  no_argument, 0,  'd' },
    {0,         0,                 0,  0 }
};

static void help(void) {
	printf("Usage: cmb-payload \n");
	printf("	-h, --help       display this help and exit\n");
	printf("	-i, --ip         setting clb listen ip address\n");
	printf("	-g, --group      setting clb listen group\n");
	printf("	-s, --speed      question speed\n");
	printf("	-l, --long       apply payload to long-link server\n");
	printf("	-t, --http       apply payload to http server\n");
	printf("	-r, --trace      trace dump\n");
	printf("	-d, --deamon     daemonize\n");
}


static bool parser_opt(int argc, char **argv) {
    int c;
	
    while (1) {
        int option_index = 0;

        c = getopt_long(argc, argv, "hlrtg:i:s:d",
                 long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
		case 'g':
			port += 10 * atoi(optarg);
		    break;
		
		case 'i':
			strncpy(ip_str, optarg, 255);
		    break;
		
        case 'l':
        	http_payload = false;
            break;

        case 't':
        	http_payload = true;
            break;

        case 'r':
        	trace_show = true;
            break;

        case 'd':
        	fork_to_background = true;
            break;

        case 's':
        	speed = atoi(optarg);
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
	rand_company(company, 0, 64);
	rand_user(user, 0, 1024);
	rand_question(question, 0, 1024);
	snprintf(content, sizeof(content)-1, format, company, user, (unsigned int)tv.tv_sec, question, msgid);
	
	string url = "localhost/wxif/";
	string response;
	
	pclient->post(url, content, response);
}


static bool rand_post(pl_clnt *sk, unsigned long msgid) {
	char company[128];
	char user[128];
	char question[128];
	struct timeval tv;
	
	gettimeofday(&tv, NULL);
	rand_company(company, 0, 64);
	rand_user(user, 0, 1024);
	rand_question(question, 0, 1024);
	
	char *content = new char[1024]();
	serial_data *pheader = (serial_data*)content;
	int datalen = snprintf((char*)(pheader->data), 1023 - sizeof(serial_data),
			 format, company, user, (unsigned int)tv.tv_sec, question, msgid);
	datalen += 1;
	pheader->length = datalen + sizeof(serial_data);
	pheader->datalen = datalen;
	bool ret = sk->ev_send_inter_thread(pheader, pheader->length);
	if (!ret) {
		delete[] content;
	}
	return ret;
}

static unsigned long total_reply = 0;
static unsigned long total_sends = 0;
static unsigned long total_valids = 0;
static struct timeval start_tv;

void count_reply(void) {
	__sync_add_and_fetch(&total_reply, 1);
}

static void *pthread_ask_http(void *args) {
	http http_client;
	unsigned long msgid = ((unsigned long)pthread_self()) << 32;
	int coeff = (speed + 1);
	unsigned int dump = 0;
	
	while (1) {
		
		/* 随机发送(100~1124)个包 */
		unsigned long send_packets = (rand() % coeff) + coeff;
		for (unsigned long cnt=0; cnt<send_packets; cnt++) {
			rand_post(&http_client, msgid++);
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
		if (((++dump & 0x01) == 0x01) && !trace_show) {
			printf("Questions: %ld, speed: %ld qps\n", questions, (questions * 10) / diff);
		}
	}
	return NULL;
}


static void *pthread_ask_simple(void *args) {
	unsigned long msgid = ((unsigned long)pthread_self()) << 32;
	int coeff = (speed + 1);
	unsigned int dump = 0;
	
	/* 创建客户端 */
	evclnt<pl_clnt> *pclnt = new evclnt<pl_clnt>(CFG_CLBSRV_IP, port + 1000);
	pl_clnt *sk = pclnt->evconnect();
	if (sk) {
		/* 启动席位对应的线程 */
    	pclnt->loop_thread();
    } else {
    	exit(1);
    }

	while (1) {
		
		/* 随机发送(100~1124)个包 */
		unsigned long send_packets = (rand() % coeff) + coeff;
		unsigned long valids = 0;
		for (unsigned long cnt=0; cnt<send_packets; cnt++) {
			if (rand_post(sk, msgid++)) {
				valids++;
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
		unsigned long send_questions = __sync_add_and_fetch(&total_valids, valids);
		if (((++dump & 0x07) == 0x07) && !trace_show) {
			printf("Send: %ld, Valids: %ld, Reply: %ld, Snd-Spd: %ld qps, Rply-Spd: %ld qps\n", 
					questions, send_questions, total_reply, (questions * 10) / diff, (total_reply * 10) / diff);
		}
		
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

