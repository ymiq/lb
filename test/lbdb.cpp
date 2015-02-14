#include <iostream>
#include <sstream>
#include <string>

#include "lb_table.h"
#include "lbdb.h"

using namespace std;

/* ���⸺�ؾ������ݿ� */
/* 
	id: 	ID
	name: 	��ҵ����
	hash:   ��ҵ����HASHֵ
	master: �����������IP
	slave:  ���ݷ�����IP
	qport:  �ͻ�������������˿�
	aport:  ��ҵӦ����������˿�
	qstat:  �ͻ�����ͳ����Ϣ
	astat:  �ͻ�����ͳ����Ϣ
	
	create table lb (id int, name varchar(256), hash bigint, master int, slave int, qport int, aport int, qstat int, astat int); 
*/

#define CFG_ENTERPRISES			128
#define CFG_ENTERPRISE_GROUPS	32
#define CFG_RECEIVE_THREADS		32

#pragma comment(lib,"libmysql.lib")  

lbdb::lbdb() {
    /* �����ļ� */
    if (mysql_library_init(0,NULL,NULL) != 0)  
    {  
        throw "mysql_library_init() error";  
    }  
    
    /* �����ݿ� */
	if (mysql_init(&mysql) == NULL) {
		throw "mysql_init() error";
	}
	
	/* ���ݿ�ѡ������ */
    if (mysql_options(&mysql, MYSQL_SET_CHARSET_NAME, "utf-8") != 0)  
    {  
        throw "mysql_options() error";  
		mysql_close(&mysql);
    }  
	
	/* �������ݿ� */
	if (mysql_real_connect(&mysql, "localhost", "root", "root", "lb_db", 3306, NULL, 0) == NULL) {
        throw "mysql_real_connect() error";  
		mysql_close(&mysql);
	}
}

lbdb::~lbdb() {
	mysql_close(&mysql);
}

int lbdb::create(void) {
	/* ������ݱ� */
	string strsql = "delete table lb;";
	if (mysql_query(&mysql, strsql.c_str()) != 0) {
        cout << "delete table error" << endl;  
		return -1;
	}
	
	/* �½��������� */
	for (int i=0; i<CFG_ENTERPRISES; i++) {
		unsigned int group = ((unsigned int)rand()) % CFG_ENTERPRISE_GROUPS;
		unsigned int qport = 0x6600 + group;
		unsigned int aport = 0x8800 + group;
		uint64_t tmp = (unsigned int)rand();
		tmp << 32;
		tmp += (unsigned int)rand();
		
		/* ������ݵ����ݿ� */
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
	    	
	    	/* ɾ����¼��*/
			string strsql = "delete table lb;";
			mysql_query(&mysql, strsql.c_str());
	    	cout << "insert into error" << endl;
	    	return -1;
	    }
	}
	return 0;
}

/* �������ݼ�¼ */
int lbdb::dump(void) {
		
	/* ��ȡ���ؾ�����Ϣ */
    MYSQL_RES *result=NULL;  
    string strsql = "select id, name, hash, master, qport, qstat from lb order by id ";  
    if (mysql_query(&mysql, strsql.c_str()) != 0)  
    {  
    	cout << "query error" << endl;
		mysql_close(&mysql);
    	return -1;
    }
	result = mysql_store_result(&mysql);  

    /* ���ؼ�¼������ */
	int rowcount = mysql_num_rows(result);  
	
	/* ȡ�ñ���ֶ����� ���� */
	unsigned int feildcount = mysql_num_fields(result);  
	
	/* �ֶ�ָ�� �����ֶ� */
	MYSQL_FIELD *feild = NULL;  
	for (unsigned int i=0; i<feildcount; i++)  
	{  
		feild = mysql_fetch_field_direct(result, i);  
		cout << feild->name << "\t";  
	}  
	cout << endl;  
	
	/* ��ָ�� ������ */
	MYSQL_ROW row =NULL;  
	while (NULL != (row = mysql_fetch_row(result)) )  
	{  
		for(int i=0; i<feildcount;i++)  
		{  
		    cout << row[i] << "\t";  
		}  
		cout << endl;  
	}  	
	
	/* �ر����ݿ� */
    mysql_free_result(result);  
}
