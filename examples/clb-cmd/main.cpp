
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include "lb_db.h"
#include "command.h"

#define CFG_LB_IPADDR		"127.0.0.1"
#define CFG_LB_CMDPORT		8000

typedef struct LB_CMD {
	unsigned int cmd;
	unsigned int group;
	unsigned long int hash; 
	unsigned int ip;
	unsigned int port;
	unsigned char data[0];
} LB_CMD;

int sockfd = -1;

static int db_create(void) {
	int ret;
	
	lb_db *db = new lb_db("localhost", 3306, "lb_db");
	ret = db->db_create();
	delete db;
	return ret; 
}

static int db_dump(void) {
	int ret;
	
	lb_db *db = new lb_db("localhost", 3306, "lb_db");
	ret = db->db_dump();
	delete db;
	return ret; 
}


static int company_hash(const char *str, uint64_t *hash) {
	uint64_t hash_val;
	
	lb_db *db = new lb_db();
	hash_val = db->check_company(str);
	delete db;
	if (hash_val && hash) {
		*hash = hash_val;
		return 0;
	}
	return -1;
}


static int group_id(const char *str, unsigned int *id) {
	unsigned int groupid = 0;
	
	/* 读取参数，获取groupid */
	errno = 0;
	if (strlen(str) >= 2) {
		if ((str[0] == '0') 
			&& ((str[1] == 'x') || (str[1] == 'X'))) {
			groupid = (unsigned int)strtoul(str, NULL, 16);
		}
	} else {
		groupid = (unsigned int)strtoul(str, NULL, 10);
	}
	if ((errno == ERANGE) || (errno == EINVAL)) {
		return -1;
	}
	
	/* 检查groupid是否有效 */
	lb_db *db = new lb_db();
	bool ret = db->check_groupid((int)groupid);
	delete db;
	if (ret && id) {
		*id = groupid;
		return 0;
	}
	
	return -1;
}


static int host_info(char *str, unsigned int *ip, unsigned int *port) {
	char *pport;
	unsigned int port_no;
	unsigned int ip_addr;
	
	/* IP字符串和端口字符串拆分 */
	pport = strstr(str, ":");
	if ((pport == NULL) || (str == pport)){
		return -1;
	}
	*pport++ = '\0';
	
	/* 获取IP地址 */
	ip_addr = (unsigned int)ntohl(inet_addr(str));
	/* 检查是否为私有地址 */
	if (((ip_addr & 0xff000000) == 0x0a000000)
		|| ((ip_addr & 0xffffff00) == 0x7f000000)
		|| (((ip_addr & 0xffff0000) >= 0xac100000)
			&& ((ip_addr & 0xffff0000) <= 0xac1f0000))
		|| ((ip_addr & 0xffff0000) == 0xc0a80000)) {
		/* 10.x.x.x
		 * 127.0.0.x
		 * 172.16.x.x-172.31.x.x
		 * 192.168.x.x 
		 */
	} else {
		return -1;
	}
	
	/* 获取端口号 */
	errno = 0;
	port_no = strtoul(pport, NULL, 10);
	if ((errno == ERANGE) || (errno == EINVAL)
		|| (port_no <= 1024) || (port_no >= 0x10000)) {
		return -1;
	}
	
	if (ip) {
		*ip = ip_addr;
	}
	if (port) {
		*port = port_no;
	}
	
	return 0;
}


static void help(void) {
	printf("db create   创建负载均衡数据库\n");
	printf("db dump     显示负载均衡数据内容\n");
	printf("stat start <company>          开启公司<company>的数据统计\n");
	printf("stat start group <groupid>    开启组<groupid>的数据统计\n");
	printf("stat stop <company>           关闭公司<company>的数据统计\n");
	printf("stat stop group <groupid>     关闭公司<company>的数据统计\n");
	printf("stat info <company>           获取公司<company>的统计信息\n");
	printf("stat info group <groupid>     获取组<groupid>的统计信息\n");
	printf("lb switch <company> <host:port>        切换公司<company>到指定服务器\n");
	printf("lb switch group <groupid> <host:port>  切换组<groupid>到指定服务器\n");
	printf("lb stop <company>          停止公司<company>的服务\n");
	printf("lb stop group <groupid>    停止组<groupid>的服务\n");
	printf("lb start <company>         开启公司<company>的服务\n");
	printf("lb start group <groupid>   开启组<groupid>的服务\n");
	printf("lb info <company>          获取公司<company>的服务器信息\n");
	printf("lb info group <groupid>    获取组<groupid>的服务器信息\n");
	printf("\n");
}


int main(int argc, char *argv[]) {
	/* 设置随机数种子 */
	srand((int)time(NULL));
	
	/* 均衡数据库命令 */
	if ((argc == 3) && !strcmp(argv[1], "db")) {
		if (!strcmp(argv[2], "create")) {
			return db_create();
		} else if (!strcmp(argv[2], "dump")) {
			return db_dump();
		} else {
			help();
			return 0;
		}
	}
	
	/* 创建命令对象 */
	command *pcmd;
	try {
		pcmd = new command;
	} catch(const char* msg) {
		printf("Error quit: %s\n", msg);
		return 0;
	}
	
	/* 统计相关命令 */
	unsigned int cmd = 0;
	unsigned int id = 0;
	unsigned long int hash = 0;
	unsigned int ip = 0;
	unsigned int port = 0;
	if ((argc > 3) && (!strcmp(argv[1], "stat"))) {
		cmd = 0x20000000;
		
		/* 统计相关命令处理 */
		if (!strcmp(argv[2], "start")) {
			cmd |= 1;
		} else if (!strcmp(argv[2], "stop")) {
			cmd |= 2;
		} else if (!strcmp(argv[2], "info")) {
			cmd |= 4;
		} else {
			help();
			return 0;
		}
		if ((argc > 4) && (!strcmp(argv[3], "group"))) { 
			cmd |= 0x10000000;
			if (group_id(argv[4], &id) < 0) {
				printf("无效组ID号\n");
				return 0;
			}
		} else {
			if (company_hash(argv[3], &hash) < 0) {
				printf("无效公司名\n");
				return 0;
			}
		}
	} else if ((argc > 3) && (!strcmp(argv[1], "lb"))) {
		cmd = 0;
		
		/* 均衡相关命令处理 */
		if (!strcmp(argv[2], "start")) {
			cmd |= 1;
		} else if (!strcmp(argv[2], "stop")) {
			cmd |= 2;
		} else if (!strcmp(argv[2], "info")) {
			cmd |= 4;
		} else if (!strcmp(argv[2], "switch")) {
			cmd |= 8;
		} else {
			help();
			return 0;
		}
		if ((argc > 4) && (!strcmp(argv[3], "group"))) { 
			cmd |= 0x10000000;
			if (group_id(argv[4], &id) < 0) {
				printf("无效组ID号\n");
				return 0;
			}
			if ((cmd & 0x0fffffff) == 0x08) {
				if (argc != 6) {
					help();
					return 0;
				}
				if (host_info(argv[4], &ip, &port) < 0) {
					printf("无效IP:PORT地址\n");
					return 0;
				}
			}
		} else {
			if (company_hash(argv[3], &hash) < 0) {
				printf("无效公司名\n");
				return 0;
			}
			if ((cmd & 0x0fffffff) == 0x08) {
				if (argc != 5) {
					help();
					return 0;
				}
				if (host_info(argv[4], &ip, &port) < 0) {
					printf("无效IP:PORT地址\n");
					return 0;
				}
			}
		}
	} else {
		help();
		return 0;
	}
	
	/* 命令处理 */
	if (pcmd->request(cmd, hash, id, ip, port) >= 0) {
		return pcmd->reponse();
	}
	return -1;
}

