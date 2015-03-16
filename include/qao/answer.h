

#ifndef __ANSWER_H__
#define __ANSWER_H__

#include <cstdlib>
#include <cstddef>
#include <string>
#include <list>
#include <json/json.h>
#include <sys/time.h>
#include <qao/qao_base.h>
#include <clb_tbl.h>
#include <hash_alg.h>

using namespace std; 

#define CFG_WX_TYPE_UNKNOW			0
#define CFG_WX_TYPE_TEXT			1
#define CFG_WX_TYPE_IMAGE			2
#define CFG_WX_TYPE_LOCATION		3
#define CFG_WX_TYPE_LINK			4
#define CFG_WX_TYPE_EVENT			5

class answer: virtual public qao_base {
public:
	answer(const char *str, size_t len);
	~answer() {};
	
	char *serialization(size_t &len);
	void dump(void);

public:
	unsigned long hash;
	unsigned long user_hash;
	unsigned long msgid;
	int type;
	struct timeval tv;
	string data;
	size_t datalen;
protected:
	
private:
	unsigned long token;
};

#endif /* __ANSWER_H__ */
