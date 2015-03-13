

#ifndef __CCTL_REQ_H__
#define __CCTL_REQ_H__

#include <cstdlib>
#include <cstddef>
#include <string>
#include <list>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <json/json.h>
#include <qao/qao_base.h>
#include <stat_man.h>
#include <clb_tbl.h>

using namespace std; 


class cctl_req: virtual public qao_base {
public:
	cctl_req(int qos = 0);
	cctl_req(const char *str, size_t len);
	~cctl_req() {};
	
	char *serialization(size_t &len);
	void dump(void);

public:
	list<unsigned long> hash_list;
	list<unsigned int> group_list;
	unsigned int command;
	unsigned int src_groupid;
	unsigned int dst_groupid;
	struct in_addr ip;
	unsigned short port;
protected:
	
private:
	unsigned long token_id;
	static unsigned int seqno;
};

#endif /* __CCTL_REQ_H__ */
