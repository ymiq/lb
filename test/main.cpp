
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "lbdb.h"

#if 0
void *receive_thread(void*) {
	/* 生成随机数 */
	
}
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                          void *(*start_routine) (void *), void *arg);
#endif

int db_create(void) {
	int ret;
	
	lbdb *db = new lbdb();
	ret = db->db_create();
	delete db;
	return ret; 
}

int db_dump(void) {
	int ret;
	
	lbdb *db = new lbdb();
	ret = db->db_dump();
	delete db;
	return ret; 
}

int lb_create(void) {
	int ret;
	
	lbdb *db = new lbdb();
	ret = db->lb_create();
	if (ret < 0) {
		printf("Can't create load balance talbe\n");
	}
	delete db;
	return ret; 
}


int stat_create(stat_table *pstat) {
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
	
	/* 创建均衡数据库 */
	if ((argc == 3) && !strcmp(argv[1], "db")
		 && !strcmp(argv[2], "create")) {
		return db_create();
	}
	
	/* 显示均衡数据库信息 */
	if ((argc == 3) && !strcmp(argv[1], "db")
		 && !strcmp(argv[2], "dump")) {
		return db_dump();
	}
	
	/* 根据数据库信息创建负载均衡HASH表 */
	if (lb_create() < 0) {
		printf("Can't create hash table\n");
	}
		
	return 0;

	/* 创建多线程，模拟用户提问 */
#if 0	
	for (int i=0; i<CFG_RECEIVE_THREADS; i++) {
		if (pthread_create(&thread_id, NULL, receive_thread, NULL) != 0) {
			printf("Create thread failed\n");
		}
	}
	
	/* 处理控制命令 */
	while (1) {
	}
	
	/* never reach here */	
	return 0;
#endif
}

