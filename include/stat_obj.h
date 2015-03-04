

#ifndef _STAT_OBJ_H__
#define _STAT_OBJ_H__

#include <cstdlib>
#include <cstddef>
#include <config.h>

using namespace std; 

#define CFG_STAT_TOTAL		(1UL<<0)		/* 统计总问题 */
#define CFG_STAT_DROP		(1UL<<1)		/* 统计丢弃的问题 */
#define CFG_STAT_ERROR		(1UL<<2)		/* 统计错误的数据包 */
#define CFG_STAT_TEXT		(1UL<<3)		/* 统计文本问题 */

#define CFG_STAT_MASK		(CFG_STAT_TOTAL | CFG_STAT_DROP | CFG_STAT_ERROR | CFG_STAT_TEXT)

typedef struct stat_info {
	unsigned long int total;
	unsigned long int errors;
	unsigned long int drops;
	unsigned long int texts;
}__attribute__((packed)) stat_info;

/* 该对象在单线程中调用，不用考虑重入问题!!! */
class stat_obj {
public:
	stat_obj();
	~stat_obj();
	
	/* 统计触发 */
	int stat(unsigned int code);
	int stat(void *packet, int packet_size);

	/* 统计操作基本方法 */
	int start(unsigned int code);	/* 开启统计 */
	int stop(unsigned int code);	/* 清除统计 */
	int clear(unsigned int code);	/* 清除统计 */
	int read(stat_info *pinfo);	/* 获取统计信息 */
	
	/* 操作符重载 */
	stat_obj& operator+=(const stat_obj *pobj);
	stat_obj& operator+=(const stat_obj &obj);
	
protected:
	
private:
	unsigned int flags;
	unsigned int clear_flags;
	stat_info info;

	int clear(unsigned int code, int record);
};

#endif /* _STAT_OBJ_H__ */
