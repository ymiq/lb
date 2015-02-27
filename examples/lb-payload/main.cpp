
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <http.h>

using namespace std;

#define CFG_SIM_THREADS		4

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
	sprintf(quest, "question(%d)", question);
}


static void rand_user(char *name, int low, int high) {
	int distance = high - low;
	if (distance < 0) {
		distance = low;
	}
	int user = (rand() % distance) + low;
	sprintf(name, "%d", user);
}

static const char format[] = 
	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
	"<company>%s</company>"
	"<user>%s</user>"
	"<question>%s</question>\n";

static void rand_post(http *pclient) {
	char company[128];
	char user[128];
	char question[128];
	char content[1024];
	
	rand_company(company, 0, 16);
	rand_user(user, 0, 16);
	rand_question(question, 0, 16);
	snprintf(content, sizeof(content)-1, format, company, user, question);
	
	string url = "wxif.lan.net/webif";
	string response;
	
	pclient->post(url, content, response);
}


static void *pthread_ask_sim(void *args) {
	http http_client;
	
	while (1) {
		
		/* 随机发送(100~1124)个包 */
		int send_packets = (rand() % 1024) + 100;
		for (int cnt=0; cnt<=send_packets; cnt++) {

			string url = "wxif.lan.net/webif";
			string post = "<xml></xml>";
			string response;
			rand_post(&http_client);
		}
		
		/* 随机Sleep一段时间(50~200ms) */
		rand_delay(1000, 2000);
	}
}


int main(int argc, char *argv[]) {
	/* 设置随机数种子 */
	srand((int)time(NULL));
	
	/* 创建多线程，模拟用户提问 */
	pthread_t th_sim[CFG_SIM_THREADS];
	for (int i=0; i<CFG_SIM_THREADS; i++) {
		if (pthread_create(&th_sim[i], NULL, pthread_ask_sim, NULL) != 0) {
			printf("Create thread failed\n");
			return -1;
		}
	}
	
	/* 处理控制命令 */
	while (1) {
		sleep(1);
	}
	
	/* never reach here */	
	return 0;
}

