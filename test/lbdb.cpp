#include <iostream>
#include <sstream>
#include <string>
#include <stdio.h>
#include "lb_table.h"
#include "lbdb.h"

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
	create table lb (id int, name varchar(256), hash bigint, master int, slave int, groupid int, qport int, aport int, qstat int, astat int); 
	create unique index lb_uidx on lb(id, hash);
	create index lb_idx on lb(master, slave, groupid, aport, qport, qstat, astat);
*/

#define CFG_ENTERPRISES			8192
#define CFG_ENTERPRISE_GROUPS	32
#define CFG_RECEIVE_THREADS		32

#pragma comment(lib,"libmysql.lib")  

lbdb::lbdb() {
    /* 检查库文件 */
    if (mysql_library_init(0, NULL, NULL) != 0)  
    {  
        throw "mysql_library_init() error";  
    }  
    
    /* 打开数据库 */
	if (mysql_init(&mysql) == NULL) {
		throw "mysql_init() error";
	}
	
	/* 数据库选项设置 */
#if 0	
    if (mysql_options(&mysql, MYSQL_SET_CHARSET_NAME, "utf-8") != 0)  
    {  
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

int lbdb::db_create(void) {

	/* 清空数据表 */
	string strsql = "truncate table lb;";
	mysql_query(&mysql, strsql.c_str());
	
	/* 新建数据内容 */
	int prev_percent = -1;
	for (int i=0; i<CFG_ENTERPRISES; i++) {
		unsigned int group = ((unsigned int)rand()) % CFG_ENTERPRISE_GROUPS;
		unsigned int qport = 0x6600 + group;
		unsigned int aport = 0x8800 + group;
		uint64_t tmp = (unsigned int)rand();
		tmp << 32;
		tmp += (unsigned int)rand();
		
		/* 添加数据到数据库 */
		stringstream name;
		name << "www."  << hex << tmp << ".com";
		stringstream sql;
		sql << "insert into lb (id, name, hash, master, slave, groupid, qport, aport, qstat, astat) values ("
				<< "0x" << hex << i << ", "
				<< "'" << name.str() << "', " 
				<< "0x" << hex << tmp << ", "
				<< "0x7f000001, 0x7f000002, "
				<< "0x" << hex << group << ", "
				<< "0x" << hex << qport << ", "
				<< "0x" << hex << aport << ", "
				<< "0, 0);";
	    if (mysql_query(&mysql, sql.str().c_str()) != 0) {
	    	
	    	/* 删除记录。*/
			string strsql = "delete table lb;";
			mysql_query(&mysql, strsql.c_str());
	    	cout << "insert into error" << endl;
	    	return -1;
	    }
	    
	    /* 显示百分比 */
	    int percent = (i * 100) / CFG_ENTERPRISES;
	    if (prev_percent != percent) {
	    	printf("\r%d%%", percent);
		    /* prev_percent = percent; */
	    }
	}
	return 0;
}

/* 返回数据记录 */
int lbdb::db_dump(void) {
		
	/* 获取负载均衡信息 */
    MYSQL_RES *result=NULL;  
    string strsql = "select id, name, hash, master, groupid, qport, qstat from lb order by id ";  
    if (mysql_query(&mysql, strsql.c_str()) != 0)  
    {  
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
	for (unsigned int i=0; i<feildcount; i++)  
	{  
		feild = mysql_fetch_field_direct(result, i);  
		cout << feild->name << "\t";  
	}  
	cout << endl;  
	
	/* 行指针 遍历行 */
	MYSQL_ROW row =NULL;  
	while (NULL != (row = mysql_fetch_row(result)) )  
	{  
		for(int i=0; i<feildcount;i++)  
		{  
		    cout << row[i] << "\t";  
		}  
		cout << endl;  
	}  	
	
	/* 关闭数据库 */
    mysql_free_result(result);  
}


int lbdb::lb_create(void) {
	return 0;
}
