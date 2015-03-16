

#ifndef __STAT_TABLE_BASE_H__
#define __STAT_TABLE_BASE_H__

#include <cstdlib>
#include <cstddef>
#include <config.h>
#include <hash_tbl.h>
#include <stat/stat_obj_base.h>

using namespace std; 

class stat_tbl_base {
public:
	stat_tbl_base() {};
	virtual ~stat_tbl_base() {};
	
	/* 统计处理函数 */
	virtual int stat(unsigned long hash, unsigned int code) = 0;
	
	virtual int start(unsigned long hash, unsigned int code) = 0;		/* 开启统计 */
	virtual int stop(unsigned long hash) = 0;							/* 暂停统计 */
	virtual int clear(unsigned long hash, unsigned int code) = 0;		/* 清除统计 */
	virtual int read(unsigned long hash, stat_info_base *pinfo) = 0;	/* 获取统计信息 */

protected:
	
private:
};

#endif /* __STAT_TABLE_BASE_H__ */
