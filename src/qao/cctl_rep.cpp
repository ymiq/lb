#include <cstdio>
#include <cstring>
#include <string>
#include <iostream>
#include <sstream>
#include <string>
#include <log.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <json/json.h>
#include <json/value.h>
#include <qao/qao_base.h>
#include <qao/cctl_rep.h>

using namespace std;


/* ==========================================================================================
 *			CLASS: cctl_rep_factory
 * ==========================================================================================
 */
cctl_rep_factory::cctl_rep_factory(const char *str, size_t len) {
	Json::Reader reader;
	Json::Value serial;
	serial_data header;
	
	if (len < sizeof(serial_data)) {
		throw "Invalid serial data";
	}
	memcpy(&header, str, sizeof(serial_data));
	init(&header);

	switch(qao_type) {
	case QAO_CCTL_REP0:
		resp = new cctl_rep0(str, len);
		break;		
	case QAO_CCTL_REP1:
		resp = new cctl_rep1(str, len);
		break;		
	case QAO_CCTL_REP2:
		resp = new cctl_rep2(str, len);
		break;
	default:
		LOGE("get qao object type: %d\n", qao_type);
		throw "unknow serial data";	
	}
}


cctl_rep_factory::~cctl_rep_factory() {
	if (resp) {
		delete resp;
	}
}


char *cctl_rep_factory::serialization(size_t &len) {
	if (resp) {
		return resp->serialization(len);
	}
	return NULL;
}


void cctl_rep_factory::dump(void) {
	if (resp) {
		resp->dump();
	}
}



/* ==========================================================================================
 *			CLASS: cctl_rep0
 * ==========================================================================================
 */
cctl_rep0::cctl_rep0(int qos) {
	init(QAO_CCTL_REP0, 0, qos);
	success = false;
}


cctl_rep0::cctl_rep0(const char *str, size_t len) {
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
	
	success = serial["success"].asBool();	
	int size = serial["resp_list"].size();
	for (int i=0; i<size; i++) {
		CCTL_REP0 resp = {0};
		
		resp.hash = serial["resp_list"][i]["hash"].asUInt64();
		resp.success = serial["resp_list"][i]["success"].asBool();
		
		Json::Value info = serial["resp_list"][i]["info"];

		resp.info.lb_status = info["lb_status"].asUInt();
		resp.info.stat_status = info["stat_status"].asUInt();
		resp.info.group = info["group"].asUInt();

		resp_list.push_back(resp);
	}
}


char *cctl_rep0::serialization(size_t &len) {
	Json::Value serial;
	Json::FastWriter writer;
		
	serial["success"] = success;	
	list<CCTL_REP0>::iterator it;
	int idx = 0;
	for (it=resp_list.begin(); it!=resp_list.end(); it++, idx++) {
		CCTL_REP0 resp = *it;
				
		serial["resp_list"][idx]["hash"] = (Json::UInt64)(resp.hash);
		serial["resp_list"][idx]["success"] = resp.success;
		
		Json::Value info;
		info["lb_status"] = resp.info.lb_status;
		info["stat_status"] = resp.info.stat_status;
		info["group"] = resp.info.group;
		serial["resp_list"][idx]["info"] = info;
	}

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


void cctl_rep0::dump(void) {
	printf("类型: %d, %s\n", QAO_CCTL_REP0, success?"操作成功":"操作失败");
		
	if (resp_list.size()) {
		list<CCTL_REP0>::iterator it;
		printf("HASH			GROUP	STATUS	LB	STAT\n");
		for (it=resp_list.begin(); it!=resp_list.end(); it++) {
			CCTL_REP0 resp = *it;
	
			printf("%016lx	%d	%s	0x%X	0x%X\n", resp.hash, resp.info.group, 
				resp.success?"OK":"FAIL", resp.info.lb_status, resp.info.stat_status);
			
		}
	}
}



/* ==========================================================================================
 *			CLASS: cctl_rep1
 * ==========================================================================================
 */
cctl_rep1::cctl_rep1(int qos) {
	init(QAO_CCTL_REP1, 0, qos);
	success = false;
}


cctl_rep1::cctl_rep1(const char *str, size_t len) {
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
	
	success = serial["success"].asBool();	
	int size = serial["resp_list"].size();
	for (int i=0; i<size; i++) {
		CCTL_REP1 resp = {0};
		
		resp.group = serial["resp_list"][i]["group"].asUInt();
		resp.success = serial["resp_list"][i]["success"].asBool();
		resp.lb_status = serial["resp_list"][i]["lb_status"].asUInt();
		resp.stat_status = serial["resp_list"][i]["stat_status"].asUInt();
		resp.ip.s_addr = serial["resp_list"][i]["ip"].asUInt();
		resp.port = (unsigned short)serial["resp_list"][i]["port"].asUInt();

		resp_list.push_back(resp);
	}
}


char *cctl_rep1::serialization(size_t &len) {
	Json::Value serial;
	Json::FastWriter writer;
		
	serial["success"] = success;	
	list<CCTL_REP1>::iterator it;
	int idx = 0;
	for (it=resp_list.begin(); it!=resp_list.end(); it++, idx++) {
		CCTL_REP1 resp = *it;
				
		serial["resp_list"][idx]["group"] = resp.group;
		serial["resp_list"][idx]["success"] = resp.success;
		serial["resp_list"][idx]["lb_status"] = resp.lb_status;
		serial["resp_list"][idx]["stat_status"] = resp.stat_status;
		serial["resp_list"][idx]["ip"] = resp.ip.s_addr;
		serial["resp_list"][idx]["port"] = resp.port;
	}

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


void cctl_rep1::dump(void) {
	printf("类型: %d, %s\n", QAO_CCTL_REP1, success?"操作成功":"操作失败");
		
	if (resp_list.size()) {
		list<CCTL_REP1>::iterator it;
		printf("GROUP		STATUS	LB	STAT	HOST\n");
		for (it=resp_list.begin(); it!=resp_list.end(); it++) {
			CCTL_REP1 resp = *it;
			char *ip_str = inet_ntoa(resp.ip);
	
			printf("%08x	%s	0x%X	0x%X	%s:%d\n", resp.group, resp.success?"OK":"FAIL", 
				resp.lb_status, resp.stat_status, ip_str, resp.port);
			
		}
	}
}



/* ==========================================================================================
 *			CLASS: cctl_rep2
 * ==========================================================================================
 */
cctl_rep2::cctl_rep2(int qos) {
	init(QAO_CCTL_REP2, 0, qos);
	success = false;
}


cctl_rep2::cctl_rep2(const char *str, size_t len) {
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
	
	success = serial["success"].asBool();	
	int size = serial["resp_list"].size();
	for (int i=0; i<size; i++) {
		CCTL_REP2 resp = {0};
		
		resp.hash = serial["resp_list"][i]["hash"].asUInt64();
		resp.success = serial["resp_list"][i]["success"].asBool();
		
		Json::Value info = serial["resp_list"][i]["info"];
		resp.info.total = info["total"].asUInt64();
		resp.info.errors = info["errors"].asUInt64();
		resp.info.drops = info["drops"].asUInt64();
		resp.info.texts = info["texts"].asUInt64();
		
		Json::Value tm = serial["resp_list"][i]["tm"];
		resp.tm.tv_sec = tm["tv_sec"].asUInt();
		resp.tm.tv_usec = tm["tv_usec"].asUInt();

		resp_list.push_back(resp);
	}
}


char *cctl_rep2::serialization(size_t &len) {
	Json::Value serial;
	Json::FastWriter writer;
	
	serial["success"] = success;	
	list<CCTL_REP2>::iterator it;
	int idx = 0;
	for (it=resp_list.begin(); it!=resp_list.end(); it++, idx++) {
		CCTL_REP2 resp = *it;
				
		serial["resp_list"][idx]["hash"] = (Json::UInt64)(resp.hash);
		serial["resp_list"][idx]["success"] = resp.success;
		
		Json::Value info;
		info["total"] = (Json::UInt64)resp.info.total;
		info["errors"] = (Json::UInt64)resp.info.errors;
		info["drops"] = (Json::UInt64)resp.info.drops;
		info["texts"] = (Json::UInt64)resp.info.texts;
		serial["resp_list"][idx]["info"] = info;
		
		Json::Value tm;
		tm["tv_sec"] = (Json::UInt)resp.tm.tv_sec;
		tm["tv_usec"] = (Json::UInt)resp.tm.tv_usec;
		serial["resp_list"][idx]["tm"] = tm;
	}
	
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


void cctl_rep2::dump(void) {
	printf("类型: %d, %s\n", QAO_CCTL_REP2, success?"操作成功":"操作失败");
	
	if (resp_list.size()) {
		list<CCTL_REP2>::iterator it;
		printf("HASH			STATUS	TOTAL	DROPS	TEXTS	ERRORS	TIME\n");
		for (it=resp_list.begin(); it!=resp_list.end(); it++) {
			CCTL_REP2 resp = *it;
	
			printf("%lx	%s	%ld	%ld	%ld	%ld	%ld\n", resp.hash, resp.success?"OK":"FAIL", 
				resp.info.total, resp.info.drops, resp.info.texts, resp.info.errors, resp.tm.tv_sec);
			
		}
	}
}



