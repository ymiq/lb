
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
#include <qao/cctl_req.h>
#include <qao/cctl_rep.h>
#include <openssl/md5.h>
#include <evclnt.h>
#include <log.h>
#include <lb_db.h>
#include "cmd_clnt.h"

/* 定义是否从数据库检查输入参数正确性 */
#define CFG_CHECK_PARAM		0
#define CFG_CMDSRV_IP		"127.0.0.1"
#define CFG_CMDSRV_PORT		8000

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


static int get_hash_list(char *str, cctl_req &req) {
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
			req.hash_list.push_back(hash_val);
		} else {
			printf("无效公司名: %s\n", pstart);
			delete db;
			return -1;
		}
#else
		hash_val = compute_hash(pstart);
		if (hash_val && (hash_val != -1UL)) {
			req.hash_list.push_back(hash_val);
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


static int get_group_list(char *str, cctl_req &req) {
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
			req.group_list.push_back(groupid);
		} else {
			printf("无效组名: %s\n", pstart);
			delete db;
			return -1;
		}
#else
		req.group_list.push_back(groupid);
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


static int get_group_id(char *str, cctl_req &req, bool src_group) {
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
			req.src_groupid = groupid;
		} else {
			req.dst_groupid = groupid;
		}
	} else {
		printf("无效组名: %s\n", str);
		delete db;
		return -1;
	}
	delete db;
#else
	if (src_group) {
		req.src_groupid = groupid;
	} else {
		req.dst_groupid = groupid;
	}
#endif
	return 0;
}


static int host_info(char *str, cctl_req &req) {
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
	
	req.ip = ip;
	req.port = port;	
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


static int param_parser(int argc, char *argv[], const char *pattern, cctl_req &req) {
	/* 组命令解析 */
	if ((argc >= 2) && (!strcmp(argv[0], "group"))) {
		req.command |= 0x10000000;
		
		/* 获取group后第一个参数 */
		if (!strcmp(pattern, "lb_create")) {
			if (get_group_id(argv[1], req, 0) < 0) {
				return 0;
			}
		} else if (!strcmp(pattern, "lb_switch")) {
			if (get_group_id(argv[1], req, 1) < 0) {
				return 0;
			}
		} else {
			if (get_group_list(argv[1], req) < 0) {
				return 0;
			}
		}
		
		/* 获取特殊命令group后第二个参数 */
		/*  1. 特殊命令 lb create group ... */
		if (!strcmp(pattern, "lb_create")) {
			if (argc < 3) {
				return 0;
			}
			if (host_info(argv[2], req) < 0) {
				return 0;
			}
		}

		/*  2. 特殊命令 lb switch group ... */
		if (!strcmp(pattern, "lb_switch")) {
			if (argc < 3) {
				return 0;
			}
			/* 获取目的Group ID */
			if (get_group_id(argv[2], req, 0) < 0) {
				return 0;
			}
		}
	} else {
		/* 获取特殊命令处理 */
		/*  1. 特殊命令 lb create <id> <names> ... */
		if (!strcmp(pattern, "lb_create")) {
			if (argc < 2) {
				return 0;
			}
			
			if (get_group_id(argv[0], req, 0) < 0) {
				return 0;
			}
			
			/* 获取第一个参数：公司名称列表 */
			if (get_hash_list(argv[1], req) < 0) {
				return 0;
			}
			return 1;
		} 
		
		/* 获取第一个参数：公司名称列表 */
		if (get_hash_list(argv[0], req) < 0) {
			return 0;
		}
		
		/* 获取特殊命令第二个参数 */
		/*  1. 特殊命令 lb switch names ... */
		if (!strcmp(pattern, "lb_switch")) {
			if ((argc < 3) || (strcmp(argv[1], "to") != 0)) {
				return 0;
			}
			if (get_group_id(argv[2], req, 0) < 0) {
				return 0;
			}
		}
	}	
	return 1;
}

static int db_parser(int argc, char *argv[]) {
	if ((argc == 2) && !strcmp(argv[0], "db")) {
		if (!strcmp(argv[1], "create")) {
			db_create();
			return 1;
		} else if (!strcmp(argv[1], "dump")) {
			db_dump();
			return 1;
		} else {
			help();
			return 1;
		}
	}
	return 0;
}


static int lb_parser(int argc, char *argv[], cctl_req &req) {
	if (argc < 2) {
		help();
		return 0;
	}
	
	argc--;
	req.command = 0;
	int ret = 0;
	if (!strcmp(argv[0], "start")) {
		req.command |= 0x01;
		ret = param_parser(argc, &argv[1], "lb_start", req);
	} else if (!strcmp(argv[0], "stop")) {
		req.command |= 0x02;
		ret = param_parser(argc, &argv[1], "lb_stop", req);
	} else if (!strcmp(argv[0], "info")) {
		req.command |= 0x04;
		ret = param_parser(argc, &argv[1], "lb_info", req);
	} else if (!strcmp(argv[0], "create")) {
		req.command |= 0x08;
		ret = param_parser(argc, &argv[1], "lb_create", req);
	} else if (!strcmp(argv[0], "delete")) {
		req.command |= 0x10;
		ret = param_parser(argc, &argv[1], "lb_delete", req);
	} else if (!strcmp(argv[0], "switch")) {
		req.command |= 0x20;
		ret = param_parser(argc, &argv[1], "lb_switch", req);
	} else if (!strcmp(argv[0], "monitor")) {
		req.command |= 0x04;
		monitor = true;
		ret = param_parser(argc, &argv[1], "lb_monitor", req);
	}
	
	if (!ret) {
		help();
		return 0;
	}
	return 1;
}


static int stat_parser(int argc, char *argv[], cctl_req &req) {
	if (argc < 2) {
		help();
		return 0;
	}

	/* 统计相关命令处理 */
	argc--;
	req.command = 0x20000000;
	int ret = 0;
	if (!strcmp(argv[0], "start")) {
		req.command |= 0x01;
		ret = param_parser(argc, &argv[1], "stat_start", req);
	} else if (!strcmp(argv[0], "stop")) {
		req.command |= 0x02;
		ret = param_parser(argc, &argv[1], "stat_stop", req);
	} else if (!strcmp(argv[0], "info")) {
		req.command |= 0x04;
		ret = param_parser(argc, &argv[1], "stat_info", req);
	} else if (!strcmp(argv[0], "clear")) {
		req.command |= 0x08;
		ret = param_parser(argc, &argv[1], "stat_create", req);
	} else if (!strcmp(argv[0], "monitor")) {
		req.command |= 0x04;
		monitor = true;
		ret = param_parser(argc, &argv[1], "stat_monitor", req);
	}
	
	if (!ret) {
		help();
		return 0;
	}
	return 1;
}


int main(int argc, char *argv[]) {
	
	LOG_CONSOLE("clb-cmd");
	
	/* 设置随机数种子 */
	srand((int)time(NULL));
	
	try {
		/* 均衡数据库命令 */
		if (db_parser(argc-1, &argv[1])) {
			return 0;
		}
	
		/* 统计相关命令 */
		cctl_req req;
		if ((argc > 3) && (!strcmp(argv[1], "stat"))) {
			if (!stat_parser(argc-2, &argv[2], req)) {
				return 0;
			}
		} else if ((argc > 3) && (!strcmp(argv[1], "lb"))) {
			if (!lb_parser(argc-2, &argv[2], req)) {
				return 0;
			}
		} else {
			help();
			return 0;
		}
		
		/* 创建基于Event的客户端，用于发送命令和接受响应 */
		evclnt<cmd_clnt> clnt(CFG_CMDSRV_IP, CFG_CMDSRV_PORT);
		cmd_clnt *sk = clnt.get_evsock();
		if (sk) {
			if (monitor) {
				sk->reg_qao(&req);
				sk->open_timer();
			}
			sk->ev_send(static_cast<qao_base *>(&req));
	    	clnt.loop();
	    }
		
	} catch(const char* msg) {
		printf("Error quit: %s\n", msg);
		return -1;
	}
	return 0;
}

