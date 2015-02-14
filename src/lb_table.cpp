#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lb_table.h"
#include <exception>


lb_table::lb_table()
{
	/* ������ */
	if (sizeof(serv_info) != sizeof(uint64_t)) {
		throw "serv_info struct is illegal.";
	}
	
	/* �����ϣ������ */
	lb_idx_buf = (lb_index *) malloc(sizeof(lb_index) * CFG_INDEX_SIZE + CFG_CACHE_ALIGN);
	if (lb_idx_buf == NULL) {
		throw "No memory for lb_idx_buf";
	}
	memset(lb_idx_buf, 0x00, sizeof(lb_index) * CFG_INDEX_SIZE + CFG_CACHE_ALIGN);
	
	/* Cache Line���봦�� */
	lb_idx = (lb_index*) (((size_t)lb_idx_buf) & ~(CFG_CACHE_ALIGN - 1));		
}


lb_table::~lb_table()
{
	/* �����ʵ�������˾Ͳ�Ҫ���������� */
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
			
			/* �������� */
			if (!pindex->items[i].hash) {
				return NULL;
			}
			
			/* ���ƥ�� */
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
				pindex->items[i] = *pitem;
				return &pindex->items[i];
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
				pindex->items[i].server = pitem->server;

				/* �����ڴ����ϣ�ȷ��д���Ⱥ�˳�� */
				mb();
				pindex->items[i].hash = hash;
				return &pindex->items[i];
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
	pindex->items[0] = *pitem;

	/* �����ڴ����ϣ�ȷ��д���Ⱥ�˳�� */
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
			
			/* �������� */
			if (!pindex->items[i].hash) {
				return false;
			}
			
			/* ���ƥ�� */
			if (pindex->items[i].hash == hash) {
				
				/* ����ɾ����� */
				pindex->items[i].hash = -1UL;
				return true;
			}
		}
		pindex = pindex->next;
	} while (pindex);
	
	return false;
}


