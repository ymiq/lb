

#ifndef _STAT_OBJ_H__
#define _STAT_OBJ_H__

#include <cstdlib>
#include <cstddef>
#include <config.h>

using namespace std; 

typedef struct stat_info {
	unsigned long total_packets;
	unsigned long error_packets;
}__attribute__((packed)) stat_info;

/* 该对象在单线程中调用，不用考虑重入问题!!! */
class stat_obj {
public:
	stat_obj();
	~stat_obj();
	
	/* 统计触发 */
	int stat();
	int stat(void *packet, int packet_size);
	int error_stat();

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
	stat_info info;
};

#endif /* _STAT_OBJ_H__ */
