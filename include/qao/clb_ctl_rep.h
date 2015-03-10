

#ifndef __CLB_CTL_REP_H__
#define __CLB_CTL_REP_H__

#include <cstdlib>
#include <cstddef>
#include <string>
#include <list>
#include <json/json.h>
#include <qao/qao_base.h>
#include <stat_man.h>
#include <clb_tbl.h>

using namespace std; 


class clb_ctl_rep: virtual public qao_base {
public:
	clb_ctl_rep() : success(false){};
	virtual ~clb_ctl_rep() {};
	
	virtual void *serialization(size_t &len, unsigned long token) = 0;
	virtual void *serialization(size_t &len) = 0;
	virtual void dump(void) = 0;

	bool success;
};


class clb_ctl_rep_factory: virtual public clb_ctl_rep {
public:
	clb_ctl_rep_factory(const char *str, size_t len);
	~clb_ctl_rep_factory();
	
	void *serialization(size_t &len, unsigned long token);
	void *serialization(size_t &len);
	void dump(void);
	
protected:
	
private:
	clb_ctl_rep *resp;
};



/* 启动/停止指定company服务 */
typedef struct CLB_CTL_REP0{
	unsigned long hash;
	bool success;
	lbsrv_info info;
}CLB_CTL_REP0;

class clb_ctl_rep0: virtual public clb_ctl_rep {
public:
	clb_ctl_rep0() {};
	clb_ctl_rep0(const char *str, size_t len);
	~clb_ctl_rep0() {};
	
	void *serialization(size_t &len, unsigned long token);
	void *serialization(size_t &len);
	void dump(void);

	list<CLB_CTL_REP0> resp_list;
};


/* 启动/停止指定group服务 */
typedef struct CLB_CTL_REP1{
	unsigned int group;
	bool success;
	int handle;
	unsigned int lb_status;	
	unsigned int stat_status;	
	unsigned int ip;
	unsigned short port;
}CLB_CTL_REP1;     

class clb_ctl_rep1: virtual public clb_ctl_rep {
public:
	clb_ctl_rep1() {};
	clb_ctl_rep1(const char *str, size_t len);
	~clb_ctl_rep1() {};
	
	void *serialization(size_t &len, unsigned long token);
	void *serialization(size_t &len);
	void dump(void);

	list<CLB_CTL_REP1> resp_list;
};


/* 启动/停止指定company服务 */
typedef struct CLB_CTL_REP2{
	unsigned long hash;
	bool success;
	stat_info info;
	struct timeval tm;
}CLB_CTL_REP2;   

class clb_ctl_rep2: virtual public clb_ctl_rep {
public:
	clb_ctl_rep2() {};
	clb_ctl_rep2(const char *str, size_t len);
	~clb_ctl_rep2() {};
	
	void *serialization(size_t &len, unsigned long token);
	void *serialization(size_t &len);
	void dump(void);
	
	list<CLB_CTL_REP2> resp_list;
};


#endif /* __CLB_CTL_REP_H__ */
