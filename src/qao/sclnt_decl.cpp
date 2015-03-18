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
#include <qao/sclnt_decl.h>
#include <utils/hash_alg.h>

using namespace std;

sclnt_decl::sclnt_decl() {
	init(QAO_SCLNT_DECL, 0, 0);
}


sclnt_decl::sclnt_decl(const char *str, size_t len) {
	Json::Reader reader;
	Json::Value serial;
	serial_data header;
	
	if (len < sizeof(serial_data)) {
		throw "Invalid serial data";
	}
	memcpy(&header, str, sizeof(serial_data));
	init(&header);

	str += sizeof(serial_data);
	if (!reader.parse(str, serial, false)) {
		throw "Json string error";
	}
	
	hash = serial["hash"].asUInt64();
	name = serial["name"].asString();
}


char *sclnt_decl::serialization(size_t &len) {
	Json::Value serial;
	Json::FastWriter writer;
	
	serial["hash"] = (Json::UInt64)hash;
	serial["name"] = name;
		
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


void sclnt_decl::dump(void) {
	LOGI("SCLNT_DECL: '%s' %lx\n", name.c_str(), hash);
}
