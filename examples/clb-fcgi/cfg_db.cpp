#include <iostream>
#include <sstream>
#include <string>
#include <cstdio>
#include <log.h>
#include <lb_table.h>
#include <lb_db.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/unistd.h>
#include <arpa/inet.h>
#include "cfg_db.h"

using namespace std;

cfg_db::cfg_db(const char *ip, unsigned short port, const char *db_name) :
			lb_db(ip, port, db_name) {
	grp_idx = (group_index *) calloc(sizeof(group_index), 1);
	if (grp_idx == NULL) {
	        throw "No memory for create lbdb";
	}
}


cfg_db::~cfg_db() {
	free(grp_idx);
}


int cfg_db::opensock(unsigned int master, int port) {
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


int cfg_db::getsock(int groupid, unsigned int master, int qport) {
	group_index *prev;
	group_index *pgrp;
	prev = pgrp = grp_idx;
	do {
		group_info *pinfo = pgrp->items;
		for (int i=0; i<CFG_GROUP_INDEX_SIZE; i++) {
			if (!pinfo->handle) {
				int handle = opensock(master, qport);
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
	
	int handle = opensock(master, qport);
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


int cfg_db::init_lb_table(lb_table *plb) {
		
	/* 获取负载均衡信息 */
    MYSQL_RES *result=NULL;  
    string strsql = "select id, name, hash, master, groupid, qport, qstat from lb order by id ";  
    if (mysql_query(&mysql, strsql.c_str()) != 0) {  
    	LOGE("query error");
    	return -1;
    }
	result = mysql_store_result(&mysql);  

	/* 行指针 遍历行 */
	MYSQL_ROW row =NULL;  
	while (NULL != (row = mysql_fetch_row(result))) {
		unsigned long int hash = strtoull(row[2], NULL, 10);
		unsigned int master = (unsigned int)strtoul(row[3], NULL, 10);
		int groupid = atoi(row[4]);
		int qport = atoi(row[5]);
		int handle = getsock(groupid, master, qport);
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



int cfg_db::init_stat_table(stat_table *pstat) {
		
	/* 获取负载均衡信息 */
    MYSQL_RES *result=NULL;  
    string strsql = "select id, name, hash, qstat from lb order by id ";  
    if (mysql_query(&mysql, strsql.c_str()) != 0) {  
    	LOGE("query error");
    	return -1;
    }
	result = mysql_store_result(&mysql);  

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
