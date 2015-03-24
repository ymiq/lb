#include <iostream>
#include <sstream>
#include <string>
#include <cstdio>
#include <utils/log.h>
#include <clb/clb_tbl.h>
#include <clb/clb_grp.h>
#include <utils/lb_db.h>
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
}


cfg_db::~cfg_db() {
}


int cfg_db::init_lb_table(clb_tbl *plb, clb_grp *pgrp) {
		
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
		unsigned long hash = strtoull(row[2], NULL, 10);
		unsigned int master = (unsigned int)strtoul(row[3], NULL, 10);
		int groupid = atoi(row[4]);
		int qport = atoi(row[5]);
		/* printf("hash: %x, master: %x, groupid: %d,  port: %x\n",
			hash, master, groupid, qport); */

		clb_grp_info grp_info;
		
		grp_info.group = groupid;
		grp_info.pclnt = NULL;
		grp_info.ip.s_addr = htonl(master);
		grp_info.port = qport;
		grp_info.lb_status = 1;
		grp_info.stat_status = 0;
		if (pgrp->create(grp_info, hash) != NULL) {
			lbsrv_info info;
	
			info.hash = hash;
			info.pclnt = grp_info.pclnt;
			info.group = groupid;
			info.lb_status = 1;
			info.stat_status = 0;
			if (plb->create(info) < 0) {
				LOGE("Can't start 0x%lx\n", hash);
				pgrp->remove(groupid, hash);
			}
		}
	}  	
	
	/* 释放查询信息 */
    mysql_free_result(result);  
    return 0;
}



int cfg_db::init_stat_table(stat_tbl_base *pstat) {
		
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
		unsigned long hash = strtoull(row[2], NULL, 10);
		int qstat = atoi(row[3]);
		/* printf("hash: %x, master: %x, groupid: %d,  port: %x\n",
			hash, master, groupid, qport); */
		if (qstat) {
			pstat->start(hash, 1);
		}
	}  	
	
	/* 释放查询信息 */
    mysql_free_result(result);  
    return 0;
}
