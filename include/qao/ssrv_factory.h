

#ifndef __SSRV_FACTORY_H__
#define __SSRV_FACTORY_H__

#include <cstdlib>
#include <cstddef>
#include <string>
#include <list>
#include <json/json.h>
#include <sys/time.h>
#include <qao/qao_base.h>
#include <qao/sclnt_decl.h>
#include <qao/answer.h>
#include <clb_tbl.h>
#include <utils/hash_alg.h>

using namespace std; 


class ssrv_factory: virtual public qao_base {
public:
	ssrv_factory();
	ssrv_factory(const char *str, size_t len);
	~ssrv_factory();
	
	char *serialization(size_t &len);
	void dump(void);

#ifdef CFG_QAO_TRACE	
	void trace(const char *fmt, ...);
	string &serial_trace(void);
	void dump_trace(void);
#endif

	sclnt_decl *get_sclnt_decl(void);
	answer *get_answer(void);

protected:
	
private:
	qao_base *qao;
};

#endif /* __SSRV_FACTORY_H__ */
