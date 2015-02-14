
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "lbdb.h"

#if 0
void *receive_thread(void*) {
	/* ��������� */
	
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

int main(int argc, char *argv[]) {
	/* ������������� */
	srand((int)time(NULL));
	
	if ((argc == 3) && !strcmp(argv[1], "db")
		 && !strcmp(argv[2], "create")) {
		return db_create();
	}
	
	if ((argc == 3) && !strcmp(argv[1], "db")
		 && !strcmp(argv[2], "dump")) {
		return db_dump();
	}
	
	return 0;

	/* �������ؾ���� */
	
	/* ����ͳ�Ʊ� */
	
	/* �����߳� */
#if 0	
	for (int i=0; i<CFG_RECEIVE_THREADS; i++) {
		if (pthread_create(&thread_id, NULL, receive_thread, NULL) != 0) {
			printf("Create thread failed\n");
		}
	}
	
	/* ����������� */
	while (1) {
	}
	
	/* never reach here */	
	return 0;
#endif
}

