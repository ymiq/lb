
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#define CFG_SIM_THREADS		4

static void *pthread_ask_sim(void *args) {
	while (1) {
		
		/* 随机发送(100~1124)个包 */
		int send_packets = (rand() % 1024) + 100;
		for (int cnt=0; cnt<=send_packets; cnt++) {
			int handle;
			
			/* 打开Socket */
			handle = 
			
			/* 发送消息 */
			
			/* 关闭Socket */
		}
		
		/* 随机Sleep一段时间(50~200ms) */
		int delay = (rand() % 150) + 50;
		usleep(delay*1000);
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

