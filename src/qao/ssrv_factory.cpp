#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <utils/log.h>
#include <sys/time.h>
#include <json/json.h>
#include <json/value.h>
#include <qao/qao_base.h>
#include <qao/ssrv_factory.h>
#include <qao/sclnt_decl.h>
#include <qao/answer.h>
#include <utils/hash_alg.h>

using namespace std;

ssrv_factory::ssrv_factory(const char *str, size_t len) {
	Json::Reader reader;
	Json::Value serial;
	serial_data header;
	
	if (len < sizeof(serial_data)) {
		throw "Invalid serial data";
	}
	memcpy(&header, str, sizeof(serial_data));
	init(&header);

	switch(qao_type) {
	case QAO_ANSWER:
		qao = new answer(str, len);
		break;		
	case QAO_SCLNT_DECL:
		qao = new sclnt_decl(str, len);
		break;		
	default:
		LOGE("get qao object type: %d\n", qao_type);
		throw "unknow serial data";	
	}
}


ssrv_factory::~ssrv_factory() {
	if (qao) {
		delete qao;
	}
}


char *ssrv_factory::serialization(size_t &len) {
	if (qao) {
		return qao->serialization(len);
	}
	return NULL;
}


void ssrv_factory::dump(void) {
	if (qao) {
		qao->dump();
	}
}


sclnt_decl *ssrv_factory::get_sclnt_decl(void) {
	return dynamic_cast<sclnt_decl *>(qao);
}


answer *ssrv_factory::get_answer(void) {
	return dynamic_cast<answer *>(qao);
}
