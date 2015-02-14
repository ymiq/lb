#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lb_table.h"
#include <exception>


lb_table::lb_table()
{
	/* 编译检查 */
	if (sizeof(serv_info) != sizeof(uint64_t)) {
		throw "serv_info struct is illegal.";
	}
	
	/* 申请哈希表索引 */
	lb_idx_buf = (lb_index *) malloc(sizeof(lb_index) * CFG_INDEX_SIZE + CFG_CACHE_ALIGN);
	if (lb_idx_buf == NULL) {
		throw "No memory for lb_idx_buf";
	}
	memset(lb_idx_buf, 0x00, sizeof(lb_index) * CFG_INDEX_SIZE + CFG_CACHE_ALIGN);
	
	/* Cache Line对齐处理 */
	lb_idx = (lb_index*) (((size_t)lb_idx_buf) & ~(CFG_CACHE_ALIGN - 1));		
}


lb_table::~lb_table()
{
	/* 该类的实例创建了就不要考虑销毁了 */
}

lb_item *lb_table::lb_get(uint64_t hash)
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
				return &pindex->items[i];
			}
		}
		pindex = pindex->next;
	} while (pindex);
	
	return NULL;
}

lb_item *lb_table::lb_update(lb_item *pitem)
{
	unsigned int index;
	lb_index *pindex, *prev;
	uint64_t hash = pitem->hash;
	
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
				pindex->items[i] = *pitem;
				return &pindex->items[i];
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
				pindex->items[i].server = pitem->server;

				/* 增加内存屏障，确保写入先后顺序 */
				mb();
				pindex->items[i].hash = hash;
				return &pindex->items[i];
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
	pindex->items[0] = *pitem;

	/* 增加内存屏障，确保写入先后顺序 */
	mb();
	prev->next = pindex;	
	return &pindex->items[0];
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
				return true;
			}
		}
		pindex = pindex->next;
	} while (pindex);
	
	return false;
}


