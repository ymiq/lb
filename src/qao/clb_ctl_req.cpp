#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <log.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <json/json.h>
#include <json/value.h>
#include <qao/qao_base.h>
#include <qao/clb_ctl_req.h>

using namespace std;


clb_ctl_req::clb_ctl_req(const char *str, size_t len): command(0), src_groupid(-1), 
			dst_groupid(-1), ip(0), port(0) {
	Json::Reader reader;
	Json::Value serial;
	serial_data header;
	
	if (len < sizeof(serial_data)) {
		throw "Invalid serial data";
	}
	memcpy(&header, str, sizeof(serial_data));
	qao_token = header.token;
	qao_type = header.type;
	str += sizeof(serial_data);
	
	if (!reader.parse(str, serial, false)) {
		throw "Json string error";
	}
	
	command = serial["command"].asUInt();
	ip = serial["ip"].asUInt();
	port = (unsigned short)serial["port"].asUInt();
	src_groupid = serial["src_groupid"].asUInt();
	dst_groupid = serial["dst_groupid"].asUInt();
	
	int size = serial["hash_list"].size();
	for (int i=0; i<size; i++) {
		hash_list.push_back(serial["hash_list"][i].asUInt64());
	}
	
	size = serial["group_list"].size();
	for (int i=0; i<size; i++) {
		group_list.push_back(serial["group_list"][i].asUInt());
	}
}


void *clb_ctl_req::serialization(size_t &len, unsigned long token) {
	Json::Value serial;
	Json::FastWriter writer;
	
	serial["command"] = command;
	serial["ip"] = ip;
	serial["port"] = port;
	serial["src_groupid"] = src_groupid;
	serial["dst_groupid"] = dst_groupid;
	
	list<unsigned long>::iterator hit;
	int idx = 0;
	for (hit=hash_list.begin(); hit!=hash_list.end(); hit++) {
		serial["hash_list"][idx++] = (Json::UInt64)(*hit);
	}
	
	list<unsigned int>::iterator git;
	idx = 0;
	for (git=group_list.begin(); git!=group_list.end(); git++) {
		serial["group_list"][idx++] = (Json::UInt)(*git);
	}
	
	string json_str = writer.write(serial);
	size_t json_len = json_str.length() + 1;
	size_t buf_len = json_len + sizeof(serial_data);
	
	void *ret = new unsigned char[buf_len];
	serial_data *pserial = (serial_data*)ret;
	pserial->token = token;
	pserial->length = buf_len;
	pserial->type = QAO_CLB_CTL_REQ;
	pserial->version = 0;
	pserial->packet_len = json_len;
	memcpy(pserial->data, json_str.c_str(), json_len);
	len = buf_len;
	return ret;
}


void *clb_ctl_req::serialization(size_t &len) {
	return serialization(len, 0);
}

