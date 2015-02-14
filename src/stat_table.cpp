#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "stat_table.h"
#include "obj_list.h"
#include "rcu_man.h"

#include <exception>

using namespace std; 

stat_table::stat_table()
{
	/* 申请哈希表索引 */
	stat_idx_buf = (stat_index *) malloc(sizeof(stat_index) * CFG_INDEX_SIZE + CFG_CACHE_ALIGN);
	if (stat_idx_buf == NULL) {
		throw "No memory for lb_idx_buf";
	}
	memset(stat_idx_buf, 0x00, sizeof(stat_index) * CFG_INDEX_SIZE + CFG_CACHE_ALIGN);
	
	/* Cache Line对齐处理 */
	stat_idx = (stat_index*) (((size_t)stat_idx_buf) & ~(CFG_CACHE_ALIGN - 1));		

	info_list = new obj_list<stat_info>();
	if (info_list == NULL) {
		throw "Can't create obj_list  for stat_table";
	}
	info_list->set_type(OBJ_LIST_TYPE_STRUCT);
	
	rcu_man *prcu = rcu_man::get_inst();
	if (prcu == NULL) {
		throw "Can't get instance of rcu_man";
	}
	prcu->obj_reg((free_list*)info_list);
}

stat_table::~stat_table()
{
	/* 该类的实例创建了就不要考虑销毁了 */
}

stat_info *stat_table::stat_get(uint64_t hash)
{
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
				pindex->items[i].stat = (stat_info*)calloc(sizeof(stat_info), 1);
				if (pindex->items[i].stat == NULL) {
					throw "No memory";
					return NULL;
				}
				return pindex->items[i].stat;
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
				pindex->items[i].stat = (stat_info*)calloc(sizeof(stat_info), 1);
				if (pindex->items[i].stat == NULL) {
					return NULL;
				}

				/* 增加内存屏障，确保写入先后顺序 */
				mb();
				pindex->items[i].hash = hash;
				return pindex->items[i].stat;
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
	pindex->items[0].stat = (stat_info*) calloc(sizeof(stat_info), 1);
	if (pindex->items[0].stat == NULL) {
		return NULL;
	}

	/* 增加内存屏障，确保写入先后顺序 */
	mb();
	prev->next = pindex;	
	return pindex->items[0].stat;
}

bool stat_table::stat_delete(uint64_t hash)
{
	unsigned int index;
	stat_index *pindex;
	stat_info *pinfo;
	
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
				stat_info *pinfo = pindex->items[i].stat;			
				pindex->items[i].stat = NULL;
				
				/* 等待释放 */
				info_list->add(pinfo);
				return true;
			}
		}
		pindex = pindex->next;
	} while (pindex);
	
	return false;
}

