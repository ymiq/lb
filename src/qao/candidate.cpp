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
#include <qao/candidate.h>
#include <utils/hash_alg.h>

using namespace std;

candidate::candidate(const char *str, size_t len) {
	Json::Reader reader;
	Json::Value serial;
	serial_data header;
	
	if (len < sizeof(serial_data)) {
		throw "Invalid serial data";
	}
	memcpy(&header, str, sizeof(serial_data));
	init(&header, QAO_CANDIDATE);

	str += sizeof(serial_data);
	if (!reader.parse(str, serial, false)) {
		throw "Json string error";
	}
	
	hash = serial["hash"].asUInt64();
	user_hash = serial["user_hash"].asUInt64();
	msgid = serial["msgid"].asUInt64();
	type = QAO_CANDIDATE;
	datalen = serial["datalen"].asUInt();
	data = "candicate " + serial["data"].asString();
}


char *candidate::serialization(size_t &len) {
	Json::Value serial;
	Json::FastWriter writer;
	
	serial["hash"] = (Json::UInt64)hash;
	serial["user_hash"] = (Json::UInt64)user_hash;
	serial["msgid"] = (Json::UInt64)msgid;
	serial["type"] = type;
	serial["datalen"] = (Json::UInt)datalen;
	serial["data"] = data;
		
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


void candidate::dump(void) {
	LOGI("CANDIDATE: '%s' FROM %lx TO %lx\n", data.c_str(), user_hash, hash);
}
