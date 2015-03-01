#include <iostream>
#include <sstream>
#include <string>
#include <cstdio>
#include <log.h>
#include <lb_table.h>
#include <lbdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/unistd.h>
#include <arpa/inet.h>

using namespace std;

/* 问题负载均衡数据库 */
/* 
	id: 	ID
	name: 	企业名称
	hash:   企业名称HASH值
	master: 主处理服务器IP
	slave:  备份服务器IP
	qport:  客户请求处理服务器端口
	aport:  企业应答处理服务器端口
	qstat:  客户请求统计信息
	astat:  客户请求统计信息
	
	create database lb_db;
	use lb_db;
	create table lb (id int, name varchar(256), hash bigint, master int, slave int, groupid int, qport int, aport int, qstat int, astat int); 
	create unique index lb_uidx on lb(id, hash);
	create index lb_idx on lb(master, slave, groupid, aport, qport, qstat, astat);
*/

#define CFG_ENTERPRISES			256
#define CFG_ENTERPRISE_GROUPS	32
#define CFG_RECEIVE_THREADS		32

lbdb::lbdb() {
    /* 检查库文件 */
    if (mysql_library_init(0, NULL, NULL) != 0) {  
        throw "mysql_library_init() error";  
    }  
    
    /* 打开数据库 */
	if (mysql_init(&mysql) == NULL) {
		throw "mysql_init() error";
	}
	
	/* 数据库选项设置 */
#if 0	
    if (mysql_options(&mysql, MYSQL_SET_CHARSET_NAME, "utf-8") != 0) {  
        throw "mysql_options() error";  
		mysql_close(&mysql);
    }  
#endif
	
	/* 链接数据库 */
	if (mysql_real_connect(&mysql, "localhost", "root", "", "lb_db", 3306, NULL, 0) == NULL) {
        throw "mysql_real_connect() error";  
		mysql_close(&mysql);
	}
	
	grp_idx = (group_index *) calloc(sizeof(group_index), 1);
	if (grp_idx == NULL) {
		throw "No memory for create lbdb";
	}
}

lbdb::~lbdb() {
	mysql_close(&mysql);
}


int lbdb::lb_opensock(unsigned int master, int port) {
	int sockfd;
	struct sockaddr_in serv_addr;
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	/* 设置连接目的地址 */
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(10000);
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
 
	/* 发送连接请求 */
	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) < 0) {
		perror("connect failed");
		return -1;
	}
	return sockfd;
}

int lbdb::lb_getsock(int groupid, unsigned int master, int qport) {
	group_index *prev;
	group_index *pgrp;
	prev = pgrp = grp_idx;
	do {
		group_info *pinfo = pgrp->items;
		for (int i=0; i<CFG_GROUP_INDEX_SIZE; i++) {
			if (!pinfo->handle) {
				int handle = lb_opensock(master, qport);
				if (handle < 0) {
					throw "Open socket failed";
				}
				pinfo->handle = handle;
				pinfo->groupid = groupid;
				pinfo->master = master;
				pinfo->port = qport;
				return handle;
			} 
			if (pinfo->groupid == groupid) {
				return pinfo->handle;
			}
			pinfo++;
		}
		prev = pgrp;
		pgrp = pgrp->next;
	} while(pgrp);
	
	pgrp = (group_index*)calloc(sizeof(group_index), 1);
	if (pgrp == NULL) {
		throw "No memory create group_index";
	}
	prev->next = pgrp;
	
	int handle = lb_opensock(master, qport);
	if (handle < 0) {
		throw "Open socket failed";
	}
	group_info *pinfo = pgrp->items;
	pinfo->handle = handle;
	pinfo->groupid = groupid;
	pinfo->master = master;
	pinfo->port = qport;
	
	return handle;
}

int lbdb::lb_init(void) {
		
	/* 获取负载均衡信息 */
    MYSQL_RES *result=NULL;  
    string strsql = "select id, name, hash, master, groupid, qport, qstat from lb order by id ";  
    if (mysql_query(&mysql, strsql.c_str()) != 0) {  
    	LOGE("query error");
    	return -1;
    }
	result = mysql_store_result(&mysql);  

    /* 返回记录集总数 */
	int rowcount = mysql_num_rows(result); 
	
	/* 取得表的字段数组 数量 */
	unsigned int feildcount = mysql_num_fields(result);  
	
	/* 获取负载均衡HASH表 */
	lb_table *plb = lb_table::get_inst();
	if (plb == NULL) {
		LOGE("Can't get load balance table");
		return -1;
	}
	
	/* 行指针 遍历行 */
	MYSQL_ROW row =NULL;  
	while (NULL != (row = mysql_fetch_row(result))) {
		unsigned long int hash = strtoull(row[2], NULL, 10);
		unsigned int master = (unsigned int)strtoul(row[3], NULL, 10);
		int groupid = atoi(row[4]);
		int qport = atoi(row[5]);
		int handle = lb_getsock(groupid, master, qport);
		/* printf("hash: %x, master: %x, groupid: %d,  port: %x, handle: %d\n",
			hash, master, groupid, qport, handle); */
		if (handle < 0) {
			LOGE("Can't open socket");
		}
		plb->lb_start(hash, handle);
	}  	
	
	/* 释放查询信息 */
    mysql_free_result(result);  
    return 0;
}



int lbdb::stat_init(stat_table *pstat) {
		
	/* 获取负载均衡信息 */
    MYSQL_RES *result=NULL;  
    string strsql = "select id, name, hash, qstat from lb order by id ";  
    if (mysql_query(&mysql, strsql.c_str()) != 0) {  
    	LOGE("query error");
    	return -1;
    }
	result = mysql_store_result(&mysql);  

    /* 返回记录集总数 */
	int rowcount = mysql_num_rows(result); 
	
	/* 取得表的字段数组 数量 */
	unsigned int feildcount = mysql_num_fields(result);  
	
	/* 行指针 遍历行 */
	MYSQL_ROW row =NULL;  
	while (NULL != (row = mysql_fetch_row(result))) {
		unsigned long int hash = strtoull(row[2], NULL, 10);
		int qstat = atoi(row[3]);
		/* printf("hash: %x, master: %x, groupid: %d,  port: %x, handle: %d\n",
			hash, master, groupid, qport, handle); */
		if (qstat) {
			pstat->open(hash);
		}
	}  	
	
	/* 释放查询信息 */
    mysql_free_result(result);  
    return 0;
}
