

#ifndef __CLB_CTL_REQ_H__
#define __CLB_CTL_REQ_H__

#include <cstdlib>
#include <cstddef>
#include <string>
#include <list>
#include <json/json.h>
#include <qao/qao_base.h>
#include <stat_man.h>
#include <clb_tbl.h>

using namespace std; 


class clb_ctl_req: virtual public qao_base {
public:
	clb_ctl_req() : command(0), src_groupid(-1), dst_groupid(-1), ip(0), port(0), seqno(0) {};
	clb_ctl_req(const char *str, size_t len);
	~clb_ctl_req() {};
	
	void *serialization(size_t &len);
	void *serialization(size_t &len, unsigned long token);

public:
	list<unsigned long> hash_list;
	list<unsigned int> group_list;
	unsigned int command;
	unsigned int src_groupid;
	unsigned int dst_groupid;
	unsigned int ip;
	unsigned short port;
	unsigned int seqno;
protected:
	
private:
		
};

#endif /* __CLB_CTL_REQ_H__ */
