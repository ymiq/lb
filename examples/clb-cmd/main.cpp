﻿
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
#include <clb_cmd.h>
#include "lb_db.h"
#include "cmd_clnt.h"

static bool monitor = false;

static int db_create(void) {
	int ret;
	
	lb_db *db;
	try {
		db = new lb_db("localhost", 3306, "lb_db");
	} catch (const char *msg) {
		printf("连接数据失败: %s\n", msg);
		return -1;
	}
	ret = db->db_create();
	delete db;
	return ret; 
}

static int db_dump(void) {
	int ret;
	
	lb_db *db;
	try {
		db = new lb_db("localhost", 3306, "lb_db");
	} catch (const char *msg) {
		printf("连接数据失败: %s\n", msg);
		return -1;
	}
	ret = db->db_dump();
	delete db;
	return ret; 
}


static int company_hash(char *str, clb_cmd &cmd) {
	unsigned long hash_val;
	
	/* 连接均衡配置数据库 */
	lb_db *db;
	try {
		db = new lb_db("localhost", 3306, "lb_db");
	} catch (const char *msg) {
		printf("连接数据失败: %s\n", msg);
		return -1;
	}
	
	/* 简单统计输入有多少个公司名称 */
	int companys = 1;
	char *pread = str;
	while (*pread) {
		if (*pread == ',') {
			companys++;
		}
		pread++;
	}
	
	/* 获取所有公司字符串, 并逐个检查是否存在于数据库中 */
	char *pstart = str;
	int valids = 0;
	while(1) {

		/* 忽略连续逗号“," */
		while (*pstart && (*pstart == ',')) {
			pstart++;
		}
		if (!*pstart) {
			if (!valids) {
				printf("无效公司名\n");
				delete db;
				return -1;
			}
			break;
		}
		
		/* 设置EOF */
		char *pend = strstr(pstart, ",");
		if (pend) {
			*pend = '\0';
		}
		
		/* 检查名称是否存在 */
		hash_val = db->check_company(pstart);
		if (hash_val) {
			cmd.hash_list.push_back(hash_val);
		} else {
			printf("无效公司名: %s\n", pstart);
			delete db;
			return -1;
		}
		valids++;
		
		/* 退出 */
		if (!pend) {
			break;
		}
		pstart = pend + 1;
	}
	return valids;
}


static int group_id(char *str, clb_cmd &cmd) {
	unsigned int groupid = 0;
	
	/* 检查groupid是否有效 */
	lb_db *db;
	try {
		db = new lb_db("localhost", 3306, "lb_db");
	} catch (const char *msg) {
		printf("连接数据失败: %s\n", msg);
		return -1;
	}

	/* 简单统计输入有多少个公司名称 */
	int groups = 1;
	char *pread = str;
	while (*pread) {
		if (*pread == ',') {
			groups++;
		}
		pread++;
	}

	/* 获取所有公司字符串, 并逐个检查是否存在于数据库中 */
	char *pstart = str;
	int valids = 0;
	while(1) {

		/* 忽略连续逗号“," */
		while (*pstart && (*pstart == ',')) {
			pstart++;
		}
		if (!*pstart) {
			if (!valids) {
				printf("无效公司名\n");
				delete db;
				return -1;
			}
			break;
		}
		
		/* 设置EOF */
		char *pend = strstr(pstart, ",");
		if (pend) {
			*pend = '\0';
		}
		
		/* 读取参数，获取groupid */
		errno = 0;
		if (strlen(pstart) >= 2) {
			if ((pstart[0] == '0') 
				&& ((pstart[1] == 'x') || (pstart[1] == 'X'))) {
				groupid = (unsigned int)strtoul(pstart, NULL, 16);
			}
		} else {
			groupid = (unsigned int)strtoul(pstart, NULL, 10);
		}
		if ((errno == ERANGE) || (errno == EINVAL)) {
			printf("无效组名: %s\n", pstart);
			delete db;
			return -1;
		}
		
		/* 检查组名称是否存在 */
		bool ret = db->check_groupid(groupid);
		if (ret) {
			cmd.group_list.push_back(groupid);
		} else {
			printf("无效组名: %s\n", pstart);
			delete db;
			return -1;
		}
		valids++;
		if (!pend) {
			break;
		}
		pstart = pend + 1;
	}
	return valids;
}


static int host_info(char *str, clb_cmd &cmd) {
	char *pport;
	unsigned int port;
	unsigned int ip;
	
	/* IP字符串和端口字符串拆分 */
	pport = strstr(str, ":");
	if ((pport == NULL) || (str == pport)){
		return -1;
	}
	*pport++ = '\0';
	
	/* 获取IP地址 */
	ip = (unsigned int)ntohl(inet_addr(str));
	/* 检查是否为私有地址 */
	if (((ip & 0xff000000) == 0x0a000000)
		|| ((ip & 0xffffff00) == 0x7f000000)
		|| (((ip & 0xffff0000) >= 0xac100000)
			&& ((ip & 0xffff0000) <= 0xac1f0000))
		|| ((ip & 0xffff0000) == 0xc0a80000)) {
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
	port = strtoul(pport, NULL, 10);
	if ((errno == ERANGE) || (errno == EINVAL)
		|| (port <= 1024) || (port >= 0x10000)) {
		return -1;
	}
	
	cmd.ip = ip;
	cmd.port = port;	
	return 0;
}


static void help(void) {
	printf("db create                   创建负载均衡数据库\n");
	printf("db dump                     显示负载均衡数据内容\n");
	printf("stat start <name>           开启公司<name>的数据统计\n");
	printf("stat start -g <id>          开启组<id>的数据统计\n");
	printf("stat stop <name>            关闭公司<name>的数据统计\n");
	printf("stat stop -g <id>           关闭公司<name>的数据统计\n");
	printf("stat info <name>            获取公司<name>的统计信息\n");
	printf("stat info -g <id>           获取组<id>的统计信息\n");
	printf("stat monitor <name>         实时显示公司<name>的统计信息\n");
	printf("stat monitor -g <id>        实时显示组<id>的统计信息\n");
	printf("stat clear <name>           清除公司<name>的统计信息\n");
	printf("stat clear -g <id>          清除组<id>的统计信息\n");
	printf("stat create -g <id> <name>  在组<id>中新建公司<name>\n");
	printf("stat create -g <id>         新建组<id>\n");
	printf("stat delete <name>          删除公司<name>\n");
	printf("stat delete -g <id>         删除组<id>\n");
	printf("lb stop <name>              停止公司<name>的服务\n");
	printf("lb stop -g <id>             停止组<id>的服务\n");
	printf("lb start <name>             开启公司<name>的服务\n");
	printf("lb start -g <id>            开启组<id>的服务\n");
	printf("lb info <name>              获取公司<name>的服务器信息\n");
	printf("lb info -g <id>             获取组<id>的服务器信息\n");
	printf("lb monitor <name>           实时显示公司<name>的服务器信息\n");
	printf("lb monitor -g <id>          实时显示组<id>的服务器信息\n");
	printf("lb create <id> <name>       在组<id>中新建<name>\n");
	printf("lb create -g <id> <host>    新建组<id>,其服务器地址为<host>(IP:PORT)\n");
	printf("lb delete <name>            删除公司<name>\n");
	printf("lb delete -g <id>           删除组<id>\n");
	printf("lb switch <name> -g <id>    切换公司<name>到指定组<id>\n");
	printf("lb switch -g <id0> <id1>    切换组<id0>中所有公司到组<id1>\n");
	printf("\n");
}


int db_parser(int argc, char *argv[]) {
	if ((argc == 3) && !strcmp(argv[1], "db")) {
		if (!strcmp(argv[2], "create")) {
			db_create();
			return 1;
		} else if (!strcmp(argv[2], "dump")) {
			db_dump();
			return 1;
		} else {
			help();
			return 1;
		}
	}
	return 0;
}


int lb_parser(int argc, char *argv[], clb_cmd &cmd) {
	unsigned int command = 0;
	
	/* 均衡相关命令处理 */
	if (!strcmp(argv[2], "start")) {
		command |= 1;
	} else if (!strcmp(argv[2], "stop")) {
		command |= 2;
	} else if (!strcmp(argv[2], "info")) {
		command |= 4;
	} else if (!strcmp(argv[2], "monitor")) {
		command |= 4;
		monitor = true;
	} else if (!strcmp(argv[2], "switch")) {
		command |= 8;
	} else {
		help();
		return 0;
	}
	if ((argc > 4) && (!strcmp(argv[3], "-g"))) { 
		command |= 0x10000000;
		if (group_id(argv[4], cmd) < 0) {
			return 0;
		}
		if ((command & 0x0fffffff) == 0x08) {
			if (argc != 6) {
				help();
				return 0;
			}
			if (host_info(argv[5], cmd) < 0) {
				printf("无效IP:PORT地址\n");
				return 0;
			}
		}
	} else {
		if (company_hash(argv[3], cmd) < 0) {
			return 0;
		}
		if ((command & 0x0fffffff) == 0x08) {
			if (argc != 5) {
				help();
				return 0;
			}
			if (host_info(argv[4], cmd) < 0) {
				printf("无效IP:PORT地址\n");
				return 0;
			}
		}
	}
	cmd.command = command;
	return 1;
}



int stat_parser(int argc, char *argv[], clb_cmd &cmd) {
	unsigned int command = 0x20000000;
	
	/* 统计相关命令处理 */
	if (!strcmp(argv[2], "start")) {
		command |= 1;
	} else if (!strcmp(argv[2], "stop")) {
		command |= 2;
	} else if (!strcmp(argv[2], "info")) {
		command |= 4;
	} else if (!strcmp(argv[2], "monitor")) {
		command |= 4;
		monitor = true;
	} else if (!strcmp(argv[2], "clear")) {
		command |= 8;
	} else {
		help();
		return 0;
	}
	if ((argc > 4) && (!strcmp(argv[3], "-g"))) { 
		command |= 0x10000000;
		if (group_id(argv[4], cmd) < 0) {
			return 0;
		}
	} else {
		if (company_hash(argv[3], cmd) < 0) {
			return 0;
		}
	}
	cmd.command = command;
	return 1;
}




int main(int argc, char *argv[]) {
	
	/* 设置随机数种子 */
	srand((int)time(NULL));
	
	/* 均衡数据库命令 */
	if (!db_parser(argc, argv)) {
		return 0;
	}
		
	
	/* 创建命令对象 */
	cmd_clnt *pclnt;
	try {
		pclnt = new cmd_clnt;
	} catch(const char* msg) {
		printf("Error quit: %s\n", msg);
		return 0;
	}
	
	/* 统计相关命令 */
	clb_cmd cmd;
	if ((argc > 3) && (!strcmp(argv[1], "stat"))) {
		if (!stat_parser(argc, argv, cmd)) {
			return 0;
		}
	} else if ((argc > 3) && (!strcmp(argv[1], "lb"))) {
		if (!lb_parser(argc, argv, cmd)) {
			return 0;
		}
	} else {
		help();
		return 0;
	}
	
	/* 命令处理 */
	if (monitor) {
		while(1) {
			if (pclnt->request(cmd) >= 0) {
				printf("\33[2J");			/* 清除屏幕内容 */
				printf("\33[1;1H");			/* 光标移到第一行第一列 */
				pclnt->reponse();
			}
			sleep(1);
		} 
	}
	if (pclnt->request(cmd) >= 0) {
		return pclnt->reponse();
	}
	return -1;
}

