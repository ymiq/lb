

#ifndef __STAT_OBJ_BASE_H__
#define __STAT_OBJ_BASE_H__

#include <cstdlib>
#include <cstddef>
#include <config.h>

using namespace std; 

/* 该对象在单线程中调用，不用考虑重入问题!!! */
class stat_info_base {
public:
	stat_info_base() {};
	virtual ~stat_info_base() {};
	
protected:	
private:
};

/* 该对象在单线程中调用，不用考虑重入问题!!! */
class stat_obj_base {
public:
	stat_obj_base() {};
	virtual ~stat_obj_base() {};
	
	/* 统计触发 */
	virtual int stat(unsigned int code) = 0;

	/* 统计操作基本方法 */
	virtual int start(unsigned int code) = 0;	/* 开启统计 */
	virtual int stop(unsigned int code) = 0;	/* 清除统计 */
	virtual int clear(unsigned int code) = 0;	/* 清除统计 */
//	virtual int read(stat_info_base *pinfo) = 0;			/* 获取统计信息 */
	
	/* 操作符重载 */
//	virtual stat_obj_base& operator+=(const stat_obj_base *pobj) = 0;
//	virtual stat_obj_base& operator+=(const stat_obj_base &obj) = 0;
	
protected:
	
private:
};

#endif /* __STAT_OBJ_BASE_H__ */
