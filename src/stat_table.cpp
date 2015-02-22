#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "rcu_obj.h"
#include "rcu_man.h"
#include "stat_obj.h"
#include "stat_table.h"
#include "stat_man.h"

#include <exception>

using namespace std; 

stat_table::stat_table() {
	/* 申请哈希表索引 */
	stat_idx_buf = (stat_index *) malloc(sizeof(stat_index) * CFG_INDEX_SIZE + CFG_CACHE_ALIGN);
	if (stat_idx_buf == NULL) {
		throw "No memory for lb_idx_buf";
	}
	memset(stat_idx_buf, 0x00, sizeof(stat_index) * CFG_INDEX_SIZE + CFG_CACHE_ALIGN);
	
	/* Cache Line对齐处理 */
	stat_idx = (stat_index*) (((size_t)stat_idx_buf) & ~(CFG_CACHE_ALIGN - 1));		
	
	/* 创建一个RCU结构体 */
	obj_list = new rcu_obj<stat_obj>();
	if (obj_list == NULL) {
		throw "Can't create rcu_obj  for stat_table";
	}
	obj_list->set_type(OBJ_LIST_TYPE_CLASS);
		
	/* 注册到统计管理器 */
	stat_man *pstat_man = stat_man::get_inst();
	if (pstat_man == NULL) {
		throw "Can't get instance of stat_man";
	}
	pstat_man->reg(this);
}


stat_table::~stat_table() {
	/* 该类的实例创建了就不要考虑销毁了 */
}


stat_obj *stat_table::stat_get(uint64_t hash) {
	unsigned int index;
	stat_index *pindex, *prev;
	
	if (!hash || (hash == -1UL)) {
		return NULL;
	}
	
	index = (unsigned int)(hash % CFG_INDEX_SIZE);
	prev = pindex = &stat_idx[index];
	
	/* 匹配是否存在该条目 */
	do {
		for (int i=0; i<CFG_ITEMS_SIZE; i++) {

			/* 搜索结束 */
			if (!pindex->items[i].hash) {
				return NULL;	
			}
			
			/* 获得匹配 */
			if (pindex->items[i].hash == hash) {
				return pindex->items[i].obj;
			}
		}
		pindex = pindex->next;
	} while (pindex);
	
	return NULL;
}


stat_obj *stat_table::stat_new(uint64_t hash) {
	unsigned int index;
	stat_index *pindex, *prev;
	
	if (!hash || (hash == -1UL)) {
		return NULL;
	}
	
	index = (unsigned int)(hash % CFG_INDEX_SIZE);
	prev = pindex = &stat_idx[index];
	
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
				pindex->items[i].obj = new stat_obj;
				if (pindex->items[i].obj == NULL) {
					throw "No memory";
					return NULL;
				}
				return pindex->items[i].obj;
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
				pindex->items[i].obj = new stat_obj;
				if (pindex->items[i].obj == NULL) {
					return NULL;
				}

				/* 增加内存屏障，确保写入先后顺序 */
				mb();
				pindex->items[i].hash = hash;
				return pindex->items[i].obj;
			}
		}
		prev = pindex;
		pindex = pindex->next;
	} while (pindex);
	
	/* 申请新的索引项, 此处未再考虑Cache对齐；因为正常情况下概率较低 */
	pindex = (stat_index*)calloc(sizeof(stat_index), 1);
	if (pindex == NULL) {
		return NULL;
	}
	
	/* 更新条目信息 */
	pindex->items[0].obj = new stat_obj;
	if (pindex->items[0].obj == NULL) {
		return NULL;
	}

	/* 增加内存屏障，确保写入先后顺序 */
	mb();
	prev->next = pindex;	
	return pindex->items[0].obj;
}


bool stat_table::stat_delete(uint64_t hash) {
	unsigned int index;
	stat_index *pindex;
	
	if (!hash || (hash == -1UL)) {
		return false;
	}
	
	index = (unsigned int)(hash % CFG_INDEX_SIZE);
	pindex = &stat_idx[index];
	
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
				
				/* 增加内存屏障，确保写入先后顺序 */
				mb();
				stat_obj *pobj = pindex->items[i].obj;			
				pindex->items[i].obj = NULL;
				
				/* 等待释放 */
				obj_list->add(pobj);
				return true;
			}
		}
		pindex = pindex->next;
	} while (pindex);
	
	return false;
}


int stat_table::open(uint64_t hash) {
	stat_obj *pobj = stat_new(hash);
	if (pobj == NULL) {
		return -1;
	}
	return 0;
}


int stat_table::close(uint64_t hash) {
	if (!stat_delete(hash))
		return -1;
	return 0;
}


int stat_table::start(uint64_t hash, uint32_t code) {
	stat_obj *pobj = stat_new(hash);
	if (!pobj) {
		return -1;
	}
	return pobj->start(code);
}


int stat_table::stop(uint64_t hash, uint32_t code) {
	stat_obj *pobj = stat_new(hash);
	if (!pobj) {
		return -1;
	}
	return pobj->stop(code);
}


int stat_table::clear(uint64_t hash, uint32_t code) {
	stat_obj *pobj = stat_new(hash);
	if (!pobj) {
		return -1;
	}
	return pobj->clear(code);
}


stat_obj *stat_table::get(uint64_t hash) {
	stat_obj *pobj = stat_new(hash);
	if (!pobj) {
		return NULL;
	}
	return pobj;
}


int stat_table::stat(uint64_t hash, void *packet, int packet_size) {
	stat_obj *pobj = stat_new(hash);
	if (!pobj) {
		return -1;
	}
	return pobj->stat(packet, packet_size);
}


int stat_table::stat(uint64_t hash) {
	stat_obj *pobj = stat_new(hash);
	if (!pobj) {
		return -1;
	}
	return pobj->stat();
}


int stat_table::error_stat(uint64_t hash) {
	stat_obj *pobj = stat_new(hash);
	if (!pobj) {
		return -1;
	}
	return pobj->error_stat();
}




