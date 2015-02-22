#include <iostream>
#include <sstream>
#include <string>
#include <stdio.h>
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
	
	create databse lb_db;
	use lb_db;
	create table lb (id int, name varchar(256), hash bigint, master int, slave int, groupid int, qport int, aport int, qstat int, astat int); 
	create unique index lb_uidx on lb(id, hash);
	create index lb_idx on lb(master, slave, groupid, aport, qport, qstat, astat);
*/

#define CFG_ENTERPRISES			256
#define CFG_ENTERPRISE_GROUPS	32
#define CFG_RECEIVE_THREADS		32

#pragma comment(lib,"libmysql.lib")  

lbdb::lbdb() {
    /* �����ļ� */
    if (mysql_library_init(0, NULL, NULL) != 0)  
    {  
        throw "mysql_library_init() error";  
    }  
    
    /* �����ݿ� */
	if (mysql_init(&mysql) == NULL) {
		throw "mysql_init() error";
	}
	
	/* ���ݿ�ѡ������ */
#if 0	
    if (mysql_options(&mysql, MYSQL_SET_CHARSET_NAME, "utf-8") != 0)  
    {  
        throw "mysql_options() error";  
		mysql_close(&mysql);
    }  
#endif
	
	/* �������ݿ� */
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

int lbdb::db_create(void) {

	/* ������ݱ� */
	string strsql = "truncate table lb;";
	mysql_query(&mysql, strsql.c_str());
	
	/* �ر��Զ��ύ */
	 mysql_autocommit(&mysql, 0);
	
	/* �½��������� */
	for (int i=0; i<CFG_ENTERPRISES; i++) {
		unsigned int group = ((unsigned int)rand()) % CFG_ENTERPRISE_GROUPS;
		unsigned int qport = 0x6600 + group;
		unsigned int aport = 0x8800 + group;
		uint64_t tmp = (unsigned int)rand();
		tmp <<= 32;
		tmp += (unsigned int)rand();
		
		/* ������ݵ����ݿ� */
		stringstream name;
		name << "www."  << hex << i << ".com";
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
	    	
	    	/* ɾ����¼��*/
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

/* �������ݼ�¼ */
int lbdb::db_dump(void) {
		
	/* ��ȡ���ؾ�����Ϣ */
    MYSQL_RES *result=NULL;  
    string strsql = "select id, name, hash, master, groupid, qport, qstat from lb order by id ";  
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
	
	/* �ͷŲ�ѯ��Ϣ */
    mysql_free_result(result);  
}

int lbdb::lb_opensock(unsigned int master, int port) {
	char name[256];
	int handle;
	
	sprintf(name, "./bin/out/x%x_%x.dat", master, port);
	handle = open(name, O_CREAT); 
	/* printf("open file: %s. handle: %d\n", name, handle); */
	return handle;
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

int lbdb::lb_create(void) {
		
	/* ��ȡ���ؾ�����Ϣ */
    MYSQL_RES *result=NULL;  
    string strsql = "select id, name, hash, master, groupid, qport, qstat from lb order by id ";  
    if (mysql_query(&mysql, strsql.c_str()) != 0)  
    {  
    	cout << "query error" << endl;
    	return -1;
    }
	result = mysql_store_result(&mysql);  

    /* ���ؼ�¼������ */
	int rowcount = mysql_num_rows(result); 
	
	/* ȡ�ñ���ֶ����� ���� */
	unsigned int feildcount = mysql_num_fields(result);  
	
	/* ��ȡ���ؾ���HASH�� */
	lb_table *plb = lb_table::get_inst();
	if (plb == NULL) {
		cout << "Can't get load balance table" << endl;
		return -1;
	}
	
	/* ��ָ�� ������ */
	MYSQL_ROW row =NULL;  
	while (NULL != (row = mysql_fetch_row(result)) )  
	{
		uint64_t hash = strtoull(row[2], NULL, 10);
		unsigned int master = (unsigned int)strtoul(row[3], NULL, 10);
		int groupid = atoi(row[4]);
		int qport = atoi(row[5]);
		int handle = lb_getsock(groupid, master, qport);
		/* printf("hash: %x, master: %x, groupid: %d,  port: %x, handle: %d\n",
			hash, master, groupid, qport, handle); */
		if (handle < 0) {
			cout << "Can't open socket" << endl;
		}
		plb->lb_start(hash, handle);
	}  	
	
	/* �ͷŲ�ѯ��Ϣ */
    mysql_free_result(result);  
    return 0;
}

int lbdb::stat_create(stat_table *pstat) {
		
	/* ��ȡ���ؾ�����Ϣ */
    MYSQL_RES *result=NULL;  
    string strsql = "select id, name, hash, qstat from lb order by id ";  
    if (mysql_query(&mysql, strsql.c_str()) != 0)  
    {  
    	cout << "query error" << endl;
    	return -1;
    }
	result = mysql_store_result(&mysql);  

    /* ���ؼ�¼������ */
	int rowcount = mysql_num_rows(result); 
	
	/* ȡ�ñ���ֶ����� ���� */
	unsigned int feildcount = mysql_num_fields(result);  
	
	/* ��ȡ���ؾ���HASH�� */
	lb_table *plb = lb_table::get_inst();
	if (plb == NULL) {
		cout << "Can't get load balance table" << endl;
		return -1;
	}
	
	/* ��ָ�� ������ */
	MYSQL_ROW row =NULL;  
	while (NULL != (row = mysql_fetch_row(result)) )  
	{
		uint64_t hash = strtoull(row[2], NULL, 10);
		int qstat = atoi(row[3]);
		/* printf("hash: %x, master: %x, groupid: %d,  port: %x, handle: %d\n",
			hash, master, groupid, qport, handle); */
		if (qstat) {
			pstat->open(hash);
		}
	}  	
	
	/* �ͷŲ�ѯ��Ϣ */
    mysql_free_result(result);  
    return 0;
}
