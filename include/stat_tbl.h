

#ifndef _STAT_TABLE_H__
#define _STAT_TABLE_H__

#include <cstdlib>
#include <cstddef>
#include <config.h>
#include <stat_obj.h>
#include <hash_tbl.h>

using namespace std; 

class stat_tbl {
public:
	stat_tbl();
	~stat_tbl();
	
	/* 统计处理函数 */
	int stat(unsigned long hash, unsigned int code);
	int stat(unsigned long hash, void *packet, int packet_size);

	int start(unsigned long hash, unsigned int code);	/* 开启统计 */
	int stop(unsigned long hash);						/* 暂停统计 */
	int clear(unsigned long hash, unsigned int code);	/* 清除统计 */
	stat_obj *get(unsigned long hash);					/* 获取对象 */

protected:
	
private:
	hash_tbl<stat_obj, CFG_INDEX_SIZE> table;
};

#endif /* _STAT_TABLE_H__ */
