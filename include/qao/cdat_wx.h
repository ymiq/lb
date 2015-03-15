

#ifndef __WX_XML_H__
#define __WX_XML_H__

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

#define CFG_TAG_DEFAULT				0
#define CFG_TAG_TOUSERNAME			1
#define CFG_TAG_FROMUSERNAME		2
#define CFG_TAG_CREATETIME			3
#define CFG_TAG_MSGTYPE				4
#define CFG_TAG_CONTENT				5
#define CFG_TAG_MSGID				6

#define CFG_WX_TYPE_UNKNOW			0
#define CFG_WX_TYPE_TEXT			1
#define CFG_WX_TYPE_IMAGE			2
#define CFG_WX_TYPE_LOCATION		3
#define CFG_WX_TYPE_LINK			4
#define CFG_WX_TYPE_EVENT			5

class cdat_wx: virtual public qao_base {
public:
	cdat_wx(const char *str, size_t len);
	~cdat_wx() {};
	
	char *serialization(size_t &len);
	void dump(void);

public:
	unsigned long hash;
	unsigned long user_hash;
	unsigned long msgid;
	int type;
	struct timeval tv;
	char *data;
	size_t datalen;
protected:
	
private:
	int tag_type;
	
	static void tag_start(void *data, const char *tag, const char **attr);
	static void tag_end(void *data, const char *tag);
	static void tag_data(void *data, const char *s, int len);
};

#endif /* __WX_XML_H__ */
