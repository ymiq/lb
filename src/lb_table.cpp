#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <exception>
#include "lb_table.h"
#include "rcu_man.h"

lb_table::lb_table()
{
	/* 编译检查 */
	if (sizeof(server_info) != sizeof(uint64_t)) {
		throw "server_info struct is illegal.";
	}
	
	/* 申请哈希表索引 */
	lb_idx_buf = (lb_index *) malloc(sizeof(lb_index) * CFG_INDEX_SIZE + CFG_CACHE_ALIGN);
	if (lb_idx_buf == NULL) {
		throw "No memory for lb_idx_buf";
	}
	memset(lb_idx_buf, 0x00, sizeof(lb_index) * CFG_INDEX_SIZE + CFG_CACHE_ALIGN);
	
	/* Cache Line对齐处理 */
	lb_idx = (lb_index*) (((size_t)lb_idx_buf) & ~(CFG_CACHE_ALIGN - 1));		
	
	/* RCU初始化 */
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
	/* 该类的实例创建了就不要考虑销毁了 */
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
			
			/* 搜索结束 */
			if (!pindex->items[i].hash) {
				return NULL;
			}
			
			/* 获得匹配 */
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
	
	/* 第一轮搜索，匹配是否存在该条目 */
	do {
		for (int i=0; i<CFG_ITEMS_SIZE; i++) {

			/* 搜索结束 */
			if (!pindex->items[i].hash) {
				goto phase2;	
			}
			
			/* 获得匹配 */
			if (pindex->items[i].hash == hash) {
				
				/* 更新条目信息 */
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
	/* 第二轮搜索，插入/更新条目 */
	pindex = prev;
	do {
		for (int i=0; i<CFG_ITEMS_SIZE; i++) {

			/* 搜索结束，创建新条目 */
			if (!pindex->items[i].hash || (pindex->items[i].hash == -1UL)) {				

				/* 更新条目信息 */
				pserver = server_info_new(NULL, handle, lb_status, stat_status);
				pindex->items[i].server = pserver;

				/* 增加内存屏障，确保写入先后顺序 */
				mb();
				pindex->items[i].hash = hash;
				return pserver;
			}
		}
		prev = pindex;
		pindex = pindex->next;
	} while (pindex);
	
	/* 申请新的索引项, 此处未再考虑Cache对齐；因为正常情况下概率较低 */
	pindex = (lb_index*)calloc(sizeof(lb_index), 1);
	if (pindex == NULL) {
		return NULL;
	}
	
	/* 更新条目信息 */
	pserver = server_info_new(NULL, handle, lb_status, stat_status);
	pindex->items[0].server = pserver;
	pindex->items[0].hash = hash;

	/* 增加内存屏障，确保写入先后顺序 */
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
			
			/* 搜索结束 */
			if (!pindex->items[i].hash) {
				return false;
			}
			
			/* 获得匹配 */
			if (pindex->items[i].hash == hash) {
				
				/* 增加删除标记 */
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


