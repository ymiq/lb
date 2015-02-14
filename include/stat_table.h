

#ifndef _STAT_TABLE_H__
#define _STAT_TABLE_H__

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include "config.h"

using namespace std; 
template<typename T> class obj_list;

typedef struct stat_info {
	unsigned long flags;
	unsigned long total_packets;
	unsigned long error_packets;
}__attribute__((packed)) stat_info;

typedef struct stat_item{
	uint64_t hash;
	stat_info *stat;
}stat_item;

typedef struct stat_index {
	stat_item  items[CFG_ITEMS_SIZE];
	stat_index *next;
}stat_index;

class stat_table {
public:
	stat_table();
	~stat_table();
	bool stat_packet(uint64_t hash);
	bool stat_error_packet(uint64_t hash);
	bool stat_merge(uint64_t hash, int size, stat_info *info);

protected:
	
private:
	stat_index *stat_idx;
	void *stat_idx_buf;
	obj_list<stat_info> *info_list;
	
	bool stat_delete(uint64_t hash);
	stat_info *stat_get(uint64_t hash);
};

#endif /* _STAT_TABLE_H__ */
