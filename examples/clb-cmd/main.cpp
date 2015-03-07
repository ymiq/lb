
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
#include <openssl/md5.h>
#include "lb_db.h"
#include "cmd_clnt.h"

/* 定义是否从数据库检查输入参数正确性 */
#define CFG_CHECK_PARAM		0

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

#if !CFG_CHECK_PARAM	
unsigned long compute_hash(const char *company) {
	MD5_CTX ctx;
	unsigned char md5[16];
	unsigned long hash = 0;
	
	MD5_Init(&ctx);
	MD5_Update(&ctx, company, strlen(company));
	MD5_Final(md5, &ctx);
	
	memcpy(&hash, md5, sizeof(hash));
	
	return hash;
}
#endif

static int get_hash_list(char *str, clb_cmd &cmd) {
	unsigned long hash_val;
	
	/* 连接均衡配置数据库 */
#if CFG_CHECK_PARAM	
	lb_db *db;
	try {
		db = new lb_db("localhost", 3306, "lb_db");
	} catch (const char *msg) {
		printf("连接数据失败: %s\n", msg);
		return -1;
	}
#endif
	
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
#if CFG_CHECK_PARAM	
				delete db;
#endif
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
#if CFG_CHECK_PARAM	
		hash_val = db->check_company(pstart);
		if (hash_val && (hash_val != -1UL)) {
			cmd.hash_list.push_back(hash_val);
		} else {
			printf("无效公司名: %s\n", pstart);
			delete db;
			return -1;
		}
#else
		hash_val = compute_hash(pstart);
		if (hash_val && (hash_val != -1UL)) {
			cmd.hash_list.push_back(hash_val);
		} else {
			printf("无效公司名: %s\n", pstart);
			return -1;
		}
#endif
		valids++;
		
		/* 退出 */
		if (!pend) {
			break;
		}
		pstart = pend + 1;
	}
#if CFG_CHECK_PARAM	
	delete db;
#endif
	return valids;
}


static int get_group_list(char *str, clb_cmd &cmd) {
	unsigned int groupid = 0;
	
	/* 检查groupid是否有效 */
#if CFG_CHECK_PARAM	
	lb_db *db;
	try {
		db = new lb_db("localhost", 3306, "lb_db");
	} catch (const char *msg) {
		printf("连接数据失败: %s\n", msg);
		return -1;
	}
#endif

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
#if CFG_CHECK_PARAM	
				delete db;
#endif
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
		if ((strlen(pstart) >= 2) && (pstart[0] == '0') 
				&& ((pstart[1] == 'x') || (pstart[1] == 'X'))) {
			groupid = (unsigned int)strtoul(pstart, NULL, 16);
		} else {
			groupid = (unsigned int)strtoul(pstart, NULL, 10);
		}
		if ((errno == ERANGE) || (errno == EINVAL)) {
			printf("无效组名: %s\n", pstart);
#if CFG_CHECK_PARAM	
			delete db;
#endif
			return -1;
		}
		
		/* 检查组名称是否存在 */
#if CFG_CHECK_PARAM	
		bool ret = db->check_groupid(groupid);
		if (ret) {
			cmd.group_list.push_back(groupid);
		} else {
			printf("无效组名: %s\n", pstart);
			delete db;
			return -1;
		}
#else
		cmd.group_list.push_back(groupid);
#endif
		valids++;
		if (!pend) {
			break;
		}
		pstart = pend + 1;
	}
#if CFG_CHECK_PARAM	
	delete db;
#endif
	return valids;
}


static int get_group_id(char *str, clb_cmd &cmd, bool src_group) {
	unsigned int groupid = 0;
	
	/* 连接均衡配置数据库 */
#if CFG_CHECK_PARAM	
	lb_db *db;
	try {
		db = new lb_db("localhost", 3306, "lb_db");
	} catch (const char *msg) {
		printf("连接数据失败: %s\n", msg);
		return -1;
	}
#endif
	
	if ((strlen(str) >= 2) && (str[0] == '0') 
			&& ((str[1] == 'x') || (str[1] == 'X'))) {
		groupid = (unsigned int)strtoul(str, NULL, 16);
	} else {
		groupid = (unsigned int)strtoul(str, NULL, 10);
	}
	if ((errno == ERANGE) || (errno == EINVAL)) {
		printf("无效组名: %s\n", str);
#if CFG_CHECK_PARAM	
		delete db;
#endif
		return -1;
	}
	
	/* 检查组名称是否存在 */
#if CFG_CHECK_PARAM	
	bool ret = db->check_groupid(groupid);
	if (ret) {
		if (src_group) {
			cmd.src_groupid = groupid;
		} else {
			cmd.dst_groupid = groupid;
		}
	} else {
		printf("无效组名: %s\n", str);
		delete db;
		return -1;
	}
	delete db;
#else
	if (src_group) {
		cmd.src_groupid = groupid;
	} else {
		cmd.dst_groupid = groupid;
	}
#endif
	return 0;
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
	printf("db create                       创建负载均衡数据库\n");
	printf("db dump                         显示负载均衡数据内容\n");
	printf("stat start <names>              开启公司<name>的数据统计\n");
	printf("stat start group <ids>          开启组<id>的数据统计\n");
	printf("stat stop <names>               关闭公司<name>的数据统计\n");
	printf("stat stop group <ids>           关闭公司<name>的数据统计\n");
	printf("stat info <names>               获取公司<name>的统计信息\n");
	printf("stat info group <ids>           获取组<id>的统计信息\n");
	printf("stat clear <names>              清除公司<name>的统计信息\n");
	printf("stat clear group <ids>          清除组<id>的统计信息\n");
	printf("stat monitor <names>            实时显示公司<name>的统计信息\n");
	printf("stat monitor group <ids>        实时显示组<id>的统计信息\n");
	printf("lb stop <names>                 停止公司<name>的服务\n");
	printf("lb stop group <ids>             停止组<id>的服务\n");
	printf("lb start <names>                开启公司<name>的服务\n");
	printf("lb start group <ids>            开启组<id>的服务\n");
	printf("lb info <names>                 获取公司<name>的服务器信息\n");
	printf("lb info group <ids>             获取组<id>的服务器信息\n");
	printf("lb create <id> <names>          在组<id>中新建<name>\n");
	printf("lb create group <id> <host>     新建组<id>,其服务器地址为<host>(IP:PORT)\n");
	printf("lb delete <names>               删除公司<name>\n");
	printf("lb delete group <ids>           删除组<id>\n");
	printf("lb switch <names> to <id>       切换公司<name>到指定组<id>\n");
	printf("lb switch group <id0> <id1>     切换组<id0>中所有公司到组<id1>\n");
	printf("lb monitor <names>              实时显示公司<name>的服务器信息\n");
	printf("lb monitor group <ids>          实时显示组<id>的服务器信息\n");
	printf("\n");
}


int param_parser(int argc, char *argv[], const char *pattern, clb_cmd &cmd) {
	/* 组命令解析 */
	if ((argc > 2) && (!strcmp(argv[0], "group"))) {
		cmd.command |= 0x10000000;
		
		/* 获取group后第一个参数 */
		if (!strcmp(pattern, "lb_create")) {
			if (get_group_id(argv[1], cmd, 0) < 0) {
				return 0;
			}
		} else if (!strcmp(pattern, "lb_switch")) {
			if (get_group_id(argv[1], cmd, 1) < 0) {
				return 0;
			}
		} else {
			if (get_group_list(argv[1], cmd) < 0) {
				return 0;
			}
		}
		
		/* 获取特殊命令group后第二个参数 */
		/*  1. 特殊命令 lb create group ... */
		if (!strcmp(pattern, "lb_create")) {
			if (argc < 3) {
				return 0;
			}
			if (host_info(argv[2], cmd) < 0) {
				return 0;
			}
		}

		/*  2. 特殊命令 lb switch group ... */
		if (!strcmp(pattern, "lb_switch")) {
			if (argc < 3) {
				return 0;
			}
			/* 获取目的Group ID */
			if (get_group_id(argv[2], cmd, 0) < 0) {
				return 0;
			}
		}
	} else {
		/* 获取第一个参数：公司名称列表 */
		if (get_hash_list(argv[0], cmd) < 0) {
			return 0;
		}
		
		/* 获取特殊命令第二个参数 */
		/*  1. 特殊命令 lb switch group ... */
		if (!strcmp(pattern, "lb_switch")) {
			if ((argc < 3) || (strcmp(argv[1], "to") != 0)) {
				return 0;
			}
			if (get_group_id(argv[2], cmd, 0) < 0) {
				return 0;
			}
		}
	}	
	return 1;
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
	if (argc < 2) {
		help();
		return 0;
	}
	
	argc--;
	cmd.command = 0;
	if (!strcmp(argv[0], "lb")) {
		cmd.command |= 0x01;
		return param_parser(argc, &argv[1], "lb_start", cmd);
	} else if (!strcmp(argv[0], "stop")) {
		cmd.command |= 0x02;
		return param_parser(argc, &argv[1], "lb_stop", cmd);
	} else if (!strcmp(argv[0], "info")) {
		cmd.command |= 0x04;
		return param_parser(argc, &argv[1], "lb_info", cmd);
	} else if (!strcmp(argv[0], "create")) {
		cmd.command |= 0x08;
		return param_parser(argc, &argv[1], "lb_create", cmd);
	} else if (!strcmp(argv[0], "delete")) {
		cmd.command |= 0x10;
		return param_parser(argc, &argv[1], "lb_delete", cmd);
	} else if (!strcmp(argv[0], "switch")) {
		cmd.command |= 0x20;
		return param_parser(argc, &argv[1], "lb_switch", cmd);
	} else if (!strcmp(argv[0], "monitor")) {
		cmd.command |= 0x04;
		monitor = true;
		return param_parser(argc, &argv[1], "lb_monitor", cmd);
	} else {
		help();
		return 0;
	}
}



int stat_parser(int argc, char *argv[], clb_cmd &cmd) {
	if (argc < 2) {
		help();
		return 0;
	}

	/* 统计相关命令处理 */
	argc--;
	cmd.command = 0x20000000;
	if (!strcmp(argv[0], "start")) {
		cmd.command |= 0x01;
		return param_parser(argc, &argv[1], "stat_start", cmd);
	} else if (!strcmp(argv[0], "stop")) {
		cmd.command |= 0x02;
		return param_parser(argc, &argv[1], "stat_stop", cmd);
	} else if (!strcmp(argv[0], "info")) {
		cmd.command |= 0x04;
		return param_parser(argc, &argv[1], "stat_info", cmd);
	} else if (!strcmp(argv[0], "clear")) {
		cmd.command |= 0x08;
		return param_parser(argc, &argv[1], "stat_create", cmd);
	} else if (!strcmp(argv[0], "monitor")) {
		cmd.command |= 0x04;
		monitor = true;
		return param_parser(argc, &argv[1], "stat_monitor", cmd);
	} else {
		help();
		return 0;
	}
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
		if (!stat_parser(argc-1, &argv[1], cmd)) {
			return 0;
		}
	} else if ((argc > 3) && (!strcmp(argv[1], "lb"))) {
		if (!lb_parser(argc-1, &argv[1], cmd)) {
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

