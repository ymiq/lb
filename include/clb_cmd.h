

#ifndef __CLB_CMD_H__
#define __CLB_CMD_H__

#include <cstdlib>
#include <cstddef>
#include <string>
#include <list>
#include <json/json.h>
#include <json_obj.h>
#include <stat_man.h>
#include <lb_table.h>

using namespace std; 


class clb_cmd: virtual public json_object {
public:
	clb_cmd():command(0), ip(0), port(0) {};
	clb_cmd(const char *str);
	~clb_cmd() {};
	clb_cmd(const clb_cmd& c); 
	
	string serialization(void);

public:
	list<unsigned long int> hash_list;
	list<unsigned int> group_list;
	unsigned int command;
	unsigned int ip;
	unsigned short port;
protected:
	
private:
		
};


class clb_cmd_resp: virtual public json_object {
public:
	clb_cmd_resp(int type, bool success): type(type), success(success) {};
	virtual ~clb_cmd_resp() {};
	
	virtual string serialization(void) = 0;
	virtual void dump(void) = 0;

public:
	unsigned int type;
	bool success;

protected:
	
private:
		
};

/* 启动/停止指定company服务 */
typedef struct CLB_CMD_RESP0{
	unsigned long int hash;
	bool success;			/* 操作成功与否标志 */
	lbsrv_info info;
}CLB_CMD_RESP0;

class clb_cmd_resp0: virtual public clb_cmd_resp {
public:
	~clb_cmd_resp0() {};
	clb_cmd_resp0(): clb_cmd_resp(0, false) {};
	clb_cmd_resp0(const char *str);
	
	string serialization(void);
	void dump(void);

public:
	list<CLB_CMD_RESP0> resp_list;

protected:
	
private:
};


/* 启动/停止指定group服务 */
typedef struct CLB_CMD_RESP1{
	unsigned long int hash;
	bool success;			/* 操作成功与否标志 */
}CLB_CMD_RESP1;

class clb_cmd_resp1: virtual public clb_cmd_resp {
public:
	~clb_cmd_resp1() {};
	clb_cmd_resp1():clb_cmd_resp(1, false){};
	clb_cmd_resp1(const char *str);
	
	string serialization(void);
	void dump(void);

public:
	list<CLB_CMD_RESP1> resp_list;

protected:
	
private:
};


/* 启动/停止指定company服务 */
typedef struct CLB_CMD_RESP100{
	unsigned long int hash;
	bool success;			/* 操作成功与否标志 */
	stat_info info;
	struct timeval tm;
}CLB_CMD_RESP100;

class clb_cmd_resp100: virtual public clb_cmd_resp {
public:
	~clb_cmd_resp100() {};
	clb_cmd_resp100():clb_cmd_resp(100, false) {};
	clb_cmd_resp100(const char *str);
	
	string serialization(void);
	void dump(void);

public:
	list<CLB_CMD_RESP100> resp_list;
	
protected:
	
private:
};


class clb_cmd_resp_factory: virtual public clb_cmd_resp {
public:
	~clb_cmd_resp_factory();
	clb_cmd_resp_factory(const char *str);
	
	string serialization(void);
	void dump(void);
	
protected:
	
private:
	clb_cmd_resp *resp;
};


#endif /* __CLB_CMD_H__ */
