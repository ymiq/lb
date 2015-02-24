
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include "lbdb.h"


static void *pthread_rcu_man(void *args) {
	
	rcu_man *prcu = rcu_man::get_inst();
	while (1) {
		/* 循环调用RCU，释放所有进入Grace Period的对象或者缓冲区 */
		prcu->job_free();
		
		/* 延时 */
		usleep(100*1000);
	}
	return NULL;
}


static void *pthread_rcv_sim(void *args) {
	while (1) {
		
		/* 随机发送(100~1124)个包 */
		int send_packets = (rand() % 1024) + 100;
		for (int cnt=0; cnt<=send_packets; cnt++) {
		}
		
		/* 随机Sleep一段时间(50~200ms) */
		int delay = (rand() % 150) + 50;
		usleep(delay*1000);
	}
}


static int lb_create(void) {
	int ret;
	
	lbdb *db = new lbdb();
	ret = db->lb_create();
	if (ret < 0) {
		printf("Can't create load balance talbe\n");
	}
	delete db;
	return ret; 
}


static int stat_create(stat_table *pstat) {
	int ret;
	
	lbdb *db = new lbdb();
	ret = db->stat_create(pstat);
	if (ret < 0) {
		printf("Can't create stat talbe\n");
	}
	delete db;
	return ret; 
}


int main(int argc, char *argv[]) {
	/* 设置随机数种子 */
	srand((int)time(NULL));
	
	/* 创建RCU管理线程 */
	pthread_t th_rcu;
	if (pthread_create(&th_rcu, NULL, pthread_rcu_man, NULL) < 0) {
		printf("can't create rcu manage thread\n");
		return -1;
	}
	
	/* 根据数据库信息创建负载均衡HASH表 */
	if (lb_create() < 0) {
		printf("Can't create hash table\n");
		return -1;
	}
		
	/* 创建多线程，模拟用户提问 */
	pthread_t th_sim[CFG_RECEIVE_THREADS];
	for (int i=0; i<CFG_RECEIVE_THREADS; i++) {
		if (pthread_create(&th_sim[i], NULL, pthread_rcv_sim, NULL) != 0) {
			printf("Create thread failed\n");
			return -1;
		}
	}
	
	/* 处理控制命令 */
	while (1) {
		
	}
	
	/* never reach here */	
	return 0;
}

