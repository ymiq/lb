#include <string.h>
#include <iostream>
#include <sstream>
#include <string>
#include <stdio.h>
#include "lbdb.h"
#include <openssl/md5.h>

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
	
	create databse lb_db;
	use lb_db;
	create table lb (id int, name varchar(256), hash bigint unsigned, master int unsigned, slave int unsigned, groupid int, qport int, aport int, qstat int unsigned, astat int unsigned); 
	create unique index lb_uidx on lb(id, hash);
	create index lb_idx on lb(master, slave, groupid, aport, qport, qstat, astat);
*/

#define CFG_ENTERPRISES			256
#define CFG_ENTERPRISE_GROUPS	32
#define CFG_RECEIVE_THREADS		32

#pragma comment(lib,"libmysql.lib")  

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
    if (mysql_options(&mysql, MYSQL_SET_CHARSET_NAME, "utf8") != 0) {  
        throw "mysql_options() error";  
		mysql_close(&mysql);
    }  
#endif
	
	/* 链接数据库 */
	if (mysql_real_connect(&mysql, "localhost", "root", "", "lb_db", 3306, NULL, 0) == NULL) {
        throw "mysql_real_connect() error";  
		mysql_close(&mysql);
	}
}

lbdb::~lbdb() {
	mysql_close(&mysql);
}

unsigned long int lbdb::compute_hash(const char *company) {
	MD5_CTX ctx;
	unsigned char md5[16];
	unsigned long int hash = 0;
	
	MD5_Init(&ctx);
	MD5_Update(&ctx, company, strlen(company));
	MD5_Final(md5, &ctx);
	
	memcpy(&hash, md5, sizeof(hash));
	
	return hash;
}

int lbdb::db_create(void) {

	/* 清空数据表 */
	string strsql = "truncate table lb;";
	mysql_query(&mysql, strsql.c_str());
	
	/* 关闭自动提交 */
	mysql_autocommit(&mysql, 0);
	
	/* 新建数据内容 */
	for (int i=0; i<CFG_ENTERPRISES; i++) {
		unsigned int group = 0;		// ((unsigned int)rand()) % CFG_ENTERPRISE_GROUPS;
		unsigned int qport = 10000 + group;
		unsigned int aport = 11000 + group;
		unsigned long int hash;
		
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
int lbdb::db_dump(void) {
		
	/* 获取负载均衡信息 */
    MYSQL_RES *result=NULL;  
    string strsql = "select id, name, hash, master, groupid, qport, qstat from lb order by id ";  
    if (mysql_query(&mysql, strsql.c_str()) != 0) {  
    	cout << "query error" << endl;
		mysql_close(&mysql);
    	return -1;
    }
	result = mysql_store_result(&mysql);  

    /* 返回记录集总数 */
	int rowcount = mysql_num_rows(result); 
	
	/* 取得表的字段数组 数量 */
	unsigned int feildcount = mysql_num_fields(result);  
	
	/* 字段指针 遍历字段 */
	MYSQL_FIELD *feild = NULL;  
	for (unsigned int i=0; i<feildcount; i++) {  
		feild = mysql_fetch_field_direct(result, i);  
		cout << feild->name << "\t\t";  
	}  
	cout << endl;  
	
	/* 行指针 遍历行 */
	MYSQL_ROW row =NULL;  
	while (NULL != (row = mysql_fetch_row(result)) ) {  
		for(int i=0; i<feildcount;i++) {  
			if (i == 2) {
				unsigned long int hash = strtoul(row[i], NULL, 10);
				cout.width(16);  
			    cout << hex << hash << "\t";  
			} else if (i == 3) {
				unsigned int master = strtoul(row[i], NULL, 10);
			    cout << hex << master << "\t";  
			} else {
		    	cout << row[i] << "\t";  
		    }
		}  
		cout << endl;  
	}  	
	
	/* 释放查询信息 */
    mysql_free_result(result);  
}


unsigned long int lbdb::get_hash(const char *company) {
		
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
			unsigned long int hash = strtoull(row[2], NULL, 10);
		    mysql_free_result(result);  
		    return hash;
		}
	}  	
	
	/* 释放查询信息 */
    mysql_free_result(result);  
    return 0;
}


bool lbdb::check_groupid(int groupid) {
		
	/* 获取负载均衡信息 */
    MYSQL_RES *result=NULL;  
    string strsql = "select id, name, hash from lb order by id ";  
    if (mysql_query(&mysql, strsql.c_str()) != 0) {  
    	cout << "query error" << endl;
    	return 0;
    }
	result = mysql_store_result(&mysql);  

	/* 行指针 遍历行 */
	MYSQL_ROW row =NULL;  
	while (NULL != (row = mysql_fetch_row(result))) {
		int gid = atoi(row[3]);
		if (gid == groupid) {
		    mysql_free_result(result);  
		    return true;
		}
	}
	
	/* 释放查询信息 */
    mysql_free_result(result);  
    return 0;
}
