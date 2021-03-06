﻿#include <string.h>
#include <iostream>
#include <sstream>
#include <string>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/md5.h>
#include <utils/lb_db.h>

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
	create table lb (id int, name varchar(256), hash bigint unsigned, master int unsigned, slave int unsigned, groupid int, qport int, aport int, qstat int unsigned, astat int unsigned); 
	create unique index lb_uidx on lb(id, hash);
	create index lb_idx on lb(master, slave, groupid, aport, qport, qstat, astat);
*/

#define CFG_ENTERPRISES			64
#define CFG_RECEIVE_THREADS		32

lb_db::lb_db(const char *ip, unsigned short port, const char *db_name) {
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
    if (mysql_options(&mysql, MYSQL_SET_CHARSET_NAME, "utf8") != 0) {  
        throw "mysql_options() error";  
		mysql_close(&mysql);
    }  
#endif
	
	/* 链接数据库 */
	if (mysql_real_connect(&mysql, ip, "root", "", db_name, port, NULL, 0) == NULL) {
        throw "mysql_real_connect() error";  
		mysql_close(&mysql);
	}
}


lb_db::~lb_db() {
	mysql_close(&mysql);
}


unsigned long lb_db::compute_hash(const char *company) {
	MD5_CTX ctx;
	unsigned char md5[16];
	unsigned long hash = 0;
	
	MD5_Init(&ctx);
	MD5_Update(&ctx, company, strlen(company));
	MD5_Final(md5, &ctx);
	
	memcpy(&hash, md5, sizeof(hash));
	
	return hash;
}


int lb_db::db_create(void) {

	/* 清空数据表 */
	string strsql = "truncate table lb;";
	mysql_query(&mysql, strsql.c_str());
	
	/* 关闭自动提交 */
	mysql_autocommit(&mysql, 0);
	
	/* 新建数据内容 */
	for (int i=0; i<CFG_ENTERPRISES; i++) {
		unsigned int group = i / 32;
		unsigned int qport = 31000 + group * 10;
		unsigned int aport = 32000 + group * 10;
		unsigned long hash;
		
		/* 添加数据到数据库 */
		stringstream name;
		name << "www."  << i << ".com";
		hash = compute_hash(name.str().c_str());
		stringstream sql;
		sql << "insert into lb (id, name, hash, master, slave, groupid, qport, aport, qstat, astat) values ("
				<< "0x" << hex << i << ", "
				<< "'" << name.str() << "', " 
				<< "0x" << hex << hash << ", "
				<< "0x7f000001, 0x7f000002, "
				<< "0x" << hex << group << ", "
				<< "0x" << hex << qport << ", "
				<< "0x" << hex << aport << ", "
				<< "0, 0);";
	    if (mysql_query(&mysql, sql.str().c_str()) != 0) {
	    	
	    	/* 删除记录。*/
			string strsql = "delete table lb;";
			mysql_query(&mysql, strsql.c_str());
			mysql_commit(&mysql);
	    	cout << "insert into error" << endl;
	    	return -1;
	    }
	}
	mysql_commit(&mysql);
	return 0;
}


/* 返回数据记录 */
int lb_db::db_dump(void) {
		
	/* 获取负载均衡信息 */
    MYSQL_RES *result=NULL;  
    string strsql = "select id, name, hash, master, groupid, qport, qstat from lb order by id ";  
    if (mysql_query(&mysql, strsql.c_str()) != 0) {  
    	cout << "query error" << endl;
		mysql_close(&mysql);
    	return -1;
    }
	result = mysql_store_result(&mysql);  

	/* 取得表的字段数组 数量 */
	unsigned int feildcount = mysql_num_fields(result);  
	
	/* 字段指针 遍历字段 */
	MYSQL_FIELD *feild = NULL;  
	const char *cltab[] = {
		"\t", "\t\t", "\t\t\t", "\t\t", "\t", "\t", "\t", "\t"
	};
	for (unsigned int i=0; i<feildcount; i++) {  
		feild = mysql_fetch_field_direct(result, i);  
		cout << feild->name << cltab[i];  
	}  
	cout << endl;  
	
	/* 行指针 遍历行 */
	MYSQL_ROW row =NULL;  
	while (NULL != (row = mysql_fetch_row(result)) ) {  
		for(unsigned int i=0; i<feildcount; i++) {  
			if (i == 2) {
				unsigned long hash = strtoul(row[i], NULL, 10);
				cout.width(16);  
			    cout << hex << hash << "\t";  
			} else if (i == 3) {
				struct in_addr in;
				in.s_addr = ntohl(strtoul(row[i], NULL, 10));
				char *ip_str = inet_ntoa(in);
			    cout << ip_str << "\t";  
			} else {
		    	cout << row[i] << "\t";  
		    }
		}  
		cout << endl;  
	}  	
	
	/* 释放查询信息 */
    mysql_free_result(result);
    return 0; 
}


unsigned long lb_db::check_company(const char *company) {
		
	/* 获取负载均衡信息 */
    MYSQL_RES *result=NULL;  
    string strsql = "select id, name, hash, groupid from lb order by id ";  
    if (mysql_query(&mysql, strsql.c_str()) != 0) {  
    	cout << "query error" << endl;
    	return 0;
    }
	result = mysql_store_result(&mysql);  

	/* 行指针 遍历行 */
	MYSQL_ROW row =NULL;  
	while (NULL != (row = mysql_fetch_row(result)))	{
		if (!strcmp(company, row[1])) {
			unsigned long hash = strtoull(row[2], NULL, 10);
		    mysql_free_result(result);  
		    return hash;
		}
	}  	
	
	/* 释放查询信息 */
    mysql_free_result(result);  
    return 0;
}


bool lb_db::check_groupid(unsigned int groupid) {
		
	/* 获取负载均衡信息 */
    MYSQL_RES *result=NULL;  
    string strsql = "select id, name, hash, groupid from lb order by id ";  
    if (mysql_query(&mysql, strsql.c_str()) != 0) {  
    	cout << "query error" << endl;
    	return 0;
    }
	result = mysql_store_result(&mysql);  

	/* 行指针 遍历行 */
	MYSQL_ROW row =NULL;  
	while (NULL != (row = mysql_fetch_row(result))) {
		unsigned int gid = strtoul(row[3], NULL, 10);
		if (gid == groupid) {
		    mysql_free_result(result);  
		    return true;
		}
	}
	
	/* 释放查询信息 */
    mysql_free_result(result);  
    return 0;
}
