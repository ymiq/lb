

#ifndef __SCLNT_DECL_H__
#define __SCLNT_DECL_H__

#include <cstdlib>
#include <cstddef>
#include <string>
#include <list>
#include <json/json.h>
#include <sys/time.h>
#include <qao/qao_base.h>
#include <clb_tbl.h>
#include <utils/hash_alg.h>

using namespace std; 


class sclnt_decl: virtual public qao_base {
public:
	sclnt_decl();
	sclnt_decl(const char *str, size_t len);
	~sclnt_decl() {};
	
	char *serialization(size_t &len);
	void dump(void);

public:
	string name;
	unsigned long hash;
	
protected:
	
private:
};

#endif /* __SCLNT_DECL_H__ */
