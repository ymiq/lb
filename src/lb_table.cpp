#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <exception>
#include "lb_table.h"
#include "rcu_man.h"

lb_table::lb_table()
{
	/* ������ */
	if (sizeof(server_info) != sizeof(uint64_t)) {
		throw "server_info struct is illegal.";
	}
	
	/* �����ϣ������ */
	lb_idx_buf = (lb_index *) malloc(sizeof(lb_index) * CFG_INDEX_SIZE + CFG_CACHE_ALIGN);
	if (lb_idx_buf == NULL) {
		throw "No memory for lb_idx_buf";
	}
	memset(lb_idx_buf, 0x00, sizeof(lb_index) * CFG_INDEX_SIZE + CFG_CACHE_ALIGN);
	
	/* Cache Line���봦�� */
	lb_idx = (lb_index*) (((size_t)lb_idx_buf) & ~(CFG_CACHE_ALIGN - 1));		
	
	/* RCU��ʼ�� */
	info_list = new rcu_obj<server_info>();
	if (info_list == NULL) {
		throw "Can't create rcu_obj for lb_table";
	}
	info_list->set_type(OBJ_LIST_TYPE_STRUCT);
	
	rcu_man *prcu = rcu_man::get_inst();
	if (prcu == NULL) {
		throw "Can't get instance of rcu_man";
	}
	prcu->obj_reg((rcu_base*)info_list);
}

lb_table::~lb_table()
{
	/* �����ʵ�������˾Ͳ�Ҫ���������� */
}

server_info *lb_table::lb_get(uint64_t hash)
{
	unsigned int index;
	lb_index *pindex;
	
	if (!hash || (hash == -1UL)) {
		return NULL;
	}
	
	index = (unsigned int)(hash % CFG_INDEX_SIZE);
	pindex = &lb_idx[index];
	
	do {
		for (int i=0; i<CFG_ITEMS_SIZE; i++) {
			
			/* �������� */
			if (!pindex->items[i].hash) {
				return NULL;
			}
			
			/* ���ƥ�� */
			if (pindex->items[i].hash == hash) {
				return pindex->items[i].server;
			}
		}
		pindex = pindex->next;
	} while (pindex);
	
	return NULL;
}

server_info *lb_table::server_info_new(server_info *ref, int handle, int lb_status, int stat_status) {
	server_info *server = (server_info *)malloc(sizeof(server_info));
	
	if (server == NULL) {
		throw "No memory to alloc server_info";
	}
	
	if (ref) {
		*server = *ref;
		info_list->add(ref);
	} else {
		server->handle = 0;
		server->lb_status = CFG_SERVER_LB_STOP;
		server->stat_status = CFG_SERVER_STAT_STOP;
	}
	
	if (handle > 0) {
		server->handle = handle;
	}
	if (lb_status >= 0) {
		if (lb_status) {
			server->lb_status = true;
		} else {
			server->lb_status = false;
		}
	}
	if (stat_status >= 0) {
		if (stat_status) {
			server->stat_status = true;
		} else {
			server->stat_status = false;
		}
	}
	return server;
}


server_info *lb_table::lb_update(uint64_t hash, int handle, int lb_status, int stat_status)
{
	unsigned int index;
	lb_index *pindex, *prev;
	server_info *pserver;
	
	if (!hash || (hash == -1UL)) {
		return NULL;
	}
	
	index = (unsigned int)(hash % CFG_INDEX_SIZE);
	prev = pindex = &lb_idx[index];
	
	/* ��һ��������ƥ���Ƿ���ڸ���Ŀ */
	do {
		for (int i=0; i<CFG_ITEMS_SIZE; i++) {

			/* �������� */
			if (!pindex->items[i].hash) {
				goto phase2;	
			}
			
			/* ���ƥ�� */
			if (pindex->items[i].hash == hash) {
				
				/* ������Ŀ��Ϣ */
				pserver = server_info_new(pindex->items[i].server, 
						handle, lb_status, stat_status);
				mb();
				pindex->items[i].server = pserver;
				return pserver;
			}
		}
		pindex = pindex->next;
	} while (pindex);
	
phase2:
	/* �ڶ�������������/������Ŀ */
	pindex = prev;
	do {
		for (int i=0; i<CFG_ITEMS_SIZE; i++) {

			/* ������������������Ŀ */
			if (!pindex->items[i].hash || (pindex->items[i].hash == -1UL)) {				

				/* ������Ŀ��Ϣ */
				pserver = server_info_new(NULL, handle, lb_status, stat_status);
				pindex->items[i].server = pserver;

				/* �����ڴ����ϣ�ȷ��д���Ⱥ�˳�� */
				mb();
				pindex->items[i].hash = hash;
				return pserver;
			}
		}
		prev = pindex;
		pindex = pindex->next;
	} while (pindex);
	
	/* �����µ�������, �˴�δ�ٿ���Cache���룻��Ϊ��������¸��ʽϵ� */
	pindex = (lb_index*)calloc(sizeof(lb_index), 1);
	if (pindex == NULL) {
		return NULL;
	}
	
	/* ������Ŀ��Ϣ */
	pserver = server_info_new(NULL, handle, lb_status, stat_status);
	pindex->items[0].server = pserver;
	pindex->items[0].hash = hash;

	/* �����ڴ����ϣ�ȷ��д���Ⱥ�˳�� */
	mb();
	prev->next = pindex;	
	return pserver;
}


bool lb_table::lb_delete(uint64_t hash)
{
	unsigned int index;
	lb_index *pindex;
	
	if (!hash || (hash == -1UL)) {
		return false;
	}
	
	index = (unsigned int)(hash % CFG_INDEX_SIZE);
	pindex = &lb_idx[index];
	
	do {
		for (int i=0; i<CFG_ITEMS_SIZE; i++) {
			
			/* �������� */
			if (!pindex->items[i].hash) {
				return false;
			}
			
			/* ���ƥ�� */
			if (pindex->items[i].hash == hash) {
				
				/* ����ɾ����� */
				pindex->items[i].hash = -1UL;
				mb();
				if (pindex->items[i].server) {
					info_list->add(pindex->items[i].server);
				}
				return true;
			}
		}
		pindex = pindex->next;
	} while (pindex);
	
	return false;
}


int lb_table::get_handle(uint64_t hash){
	server_info *pserver;
	
	pserver = lb_get(hash);
	if (pserver == NULL) {
		return -1;
	}
	return pserver->handle;
}

int lb_table::get_handle(uint64_t hash, bool *lb_status) {
	server_info *pserver;
	
	pserver = lb_get(hash);
	if (pserver == NULL) {
		return -1;
	}
	if (lb_status) {
		*lb_status = pserver->lb_status;
	}
	return pserver->handle;
}

bool lb_table::is_lb_start(uint64_t hash) {
	server_info *pserver;
	
	pserver = lb_get(hash);
	if (pserver == NULL) {
		return false;
	}
	return pserver->lb_status;
}

int lb_table::lb_start(uint64_t hash) {
	server_info *pserver;
	
	pserver = lb_update(hash, -1, CFG_SERVER_LB_START, -1);
	if (pserver == NULL) {
		return -1;
	}
	return 0;
}

int lb_table::lb_start(uint64_t hash, int handle) {
	server_info *pserver;
	
	pserver = lb_update(hash, handle, CFG_SERVER_LB_START, -1);
	if (pserver == NULL) {
		return -1;
	}
	return 0;
}

int lb_table::lb_stop(uint64_t hash) {
	server_info *pserver;
	
	pserver = lb_update(hash, -1, CFG_SERVER_LB_STOP, -1);
	if (pserver == NULL) {
		return -1;
	}
	return 0;
}

int lb_table::lb_stop(uint64_t hash, int handle) {
	server_info *pserver;
	
	pserver = lb_update(hash, handle, CFG_SERVER_LB_STOP, -1);
	if (pserver == NULL) {
		return -1;
	}
	return 0;
}

bool lb_table::is_stat_start(uint64_t hash) {
	server_info *pserver;
	
	pserver = lb_get(hash);
	if (pserver == NULL) {
		return false;
	}
	return pserver->stat_status;
}

int lb_table::stat_start(uint64_t hash) {
	server_info *pserver;
	
	pserver = lb_update(hash, -1, -1, CFG_SERVER_STAT_START);
	if (pserver == NULL) {
		return -1;
	}
	return 0;
}

int lb_table::stat_stop(uint64_t hash) {
	server_info *pserver;
	
	pserver = lb_update(hash, -1, -1, CFG_SERVER_STAT_STOP);
	if (pserver == NULL) {
		return -1;
	}
	return 0;
}


