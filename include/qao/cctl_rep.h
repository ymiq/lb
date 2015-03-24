

#ifndef __CCTL_REP_H__
#define __CCTL_REP_H__

#include <cstdlib>
#include <cstddef>
#include <string>
#include <list>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <json/json.h>
#include <qao/qao_base.h>
#include <stat/clb_stat.h>
#include <clb/clb_tbl.h>

using namespace std; 


class cctl_rep_factory: virtual public qao_base {
public:
	cctl_rep_factory(const char *str, size_t len);
	~cctl_rep_factory();
	
	char *serialization(size_t &len);
	void dump(void);
	
protected:
	
private:
	qao_base *resp;
};



/* 启动/停止指定company服务 */
typedef struct CCTL_REP0{
	unsigned long hash;
	bool success;
	lbsrv_info info;
}CCTL_REP0;

class cctl_rep0: virtual public qao_base {
public:
	cctl_rep0(int qos = 0);
	cctl_rep0(const char *str, size_t len);
	~cctl_rep0() {};
	
	char *serialization(size_t &len);
	void dump(void);

	list<CCTL_REP0> resp_list;
	bool success;
};


/* 启动/停止指定group服务 */
typedef struct CCTL_REP1{
	unsigned int group;
	bool success;
	unsigned int lb_status;	
	unsigned int stat_status;	
	struct in_addr ip;
	unsigned short port;
}CCTL_REP1;     

class cctl_rep1: virtual public qao_base {
public:
	cctl_rep1(int qos = 0);
	cctl_rep1(const char *str, size_t len);
	~cctl_rep1() {};
	
	char *serialization(size_t &len);
	void dump(void);

	list<CCTL_REP1> resp_list;
	bool success;
};


/* 启动/停止指定company服务 */
typedef struct CCTL_REP2{
	unsigned long hash;
	bool success;
	stat_info info;
	struct timeval tm;
}CCTL_REP2;   

class cctl_rep2: virtual public qao_base {
public:
	cctl_rep2(int qos = 0);
	cctl_rep2(const char *str, size_t len);
	~cctl_rep2() {};
	
	char *serialization(size_t &len);
	void dump(void);
	
	list<CCTL_REP2> resp_list;
	bool success;
};


#endif /* __CCTL_REP_H__ */
