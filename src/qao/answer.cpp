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
#include <qao/answer.h>
#include <utils/hash_alg.h>

using namespace std;

answer::answer(const char *str, size_t len) {
	Json::Reader reader;
	Json::Value serial;
	serial_data header;
	
	if (len < sizeof(serial_data)) {
		throw "Invalid serial data";
	}
	memcpy(&header, str, sizeof(serial_data));
	init(&header, QAO_ANSWER);

	str += sizeof(serial_data);
	if (!reader.parse(str, serial, false)) {
		throw "Json string error";
	}
	
	hash = serial["hash"].asUInt64();
	user_hash = serial["user_hash"].asUInt64();
	msgid = serial["msgid"].asUInt64();
	type = QAO_ANSWER;
	datalen = serial["datalen"].asUInt();
	data = serial["data"].asString();
#ifdef CFG_QAO_TRACE
	string trace = serial["trace"].asString();
	init(trace);
#endif
}


char *answer::serialization(size_t &len) {
	Json::Value serial;
	Json::FastWriter writer;
	
	serial["hash"] = (Json::UInt64)hash;
	serial["user_hash"] = (Json::UInt64)user_hash;
	serial["msgid"] = (Json::UInt64)msgid;
	serial["type"] = type;
	serial["datalen"] = (Json::UInt)datalen;
	serial["data"] = data;
#ifdef CFG_QAO_TRACE	
	serial["trace"] = serial_trace();
#endif
		
	string json_str = writer.write(serial);
	size_t json_len = json_str.length() + 1;
	size_t buf_len = json_len + sizeof(serial_data);
	
	char *ret = new char[buf_len];
	serial_data *pserial = (serial_data*)ret;
	serial_header(pserial, buf_len, json_len);
	memcpy(pserial->data, json_str.c_str(), json_len);
	len = buf_len;
	return ret;
}


void answer::dump(void) {
	LOGI("ANSWER: '%s' FROM %lx TO %lx\n", data.c_str(), user_hash, hash);
}


void answer::dump(const char *fmt, ...) {
    va_list ap;
    char buf[1024];

    va_start(ap, fmt);
    vsnprintf(buf, 1023, fmt, ap);
    va_end(ap);
    
	LOGI("ANSWER(%s): '%s' FROM %lx TO %lx\n", buf, data.c_str(), user_hash, hash);
}
