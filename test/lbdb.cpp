#include <iostream>
#include <sstream>
#include <string>

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
	
	create table lb (id int, name varchar(256), hash bigint, master int, slave int, qport int, aport int, qstat int, astat int); 
*/

#define CFG_ENTERPRISES			128
#define CFG_ENTERPRISE_GROUPS	32
#define CFG_RECEIVE_THREADS		32

#pragma comment(lib,"libmysql.lib")  

lbdb::lbdb() {
    /* 检查库文件 */
    if (mysql_library_init(0,NULL,NULL) != 0)  
    {  
        throw "mysql_library_init() error";  
    }  
    
    /* 打开数据库 */
	if (mysql_init(&mysql) == NULL) {
		throw "mysql_init() error";
	}
	
	/* 数据库选项设置 */
    if (mysql_options(&mysql, MYSQL_SET_CHARSET_NAME, "utf-8") != 0)  
    {  
        throw "mysql_options() error";  
		mysql_close(&mysql);
    }  
	
	/* 链接数据库 */
	if (mysql_real_connect(&mysql, "localhost", "root", "root", "lb_db", 3306, NULL, 0) == NULL) {
        throw "mysql_real_connect() error";  
		mysql_close(&mysql);
	}
}

lbdb::~lbdb() {
	mysql_close(&mysql);
}

int lbdb::create(void) {
	/* 清空数据表 */
	string strsql = "delete table lb;";
	if (mysql_query(&mysql, strsql.c_str()) != 0) {
        cout << "delete table error" << endl;  
		return -1;
	}
	
	/* 新建数据内容 */
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
		sql << "insert into lb (id, name, hash, master, slave, qport, aport, qstat, astat) values ("
				<< hex << i << ", "
				<< "'" << name << "', " 
				<< hex << tmp << ", "
				<< "0x7f000001, 0x7f000002"
				<< hex << qport << ", "
				<< hex << aport << ", "
				<< "0, 0);";
	    if (mysql_query(&mysql, sql.str().c_str()) != 0) {
	    	
	    	/* 删除记录。*/
			string strsql = "delete table lb;";
			mysql_query(&mysql, strsql.c_str());
	    	cout << "insert into error" << endl;
	    	return -1;
	    }
	}
	return 0;
}

/* 返回数据记录 */
int lbdb::dump(void) {
		
	/* 获取负载均衡信息 */
    MYSQL_RES *result=NULL;  
    string strsql = "select id, name, hash, master, qport, qstat from lb order by id ";  
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
