#include <cstdio>
#include <string>
#include <iostream>
#include <sstream>
#include <string>
#include <log.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <json/json.h>
#include <json/value.h>
#include "clb_cmd.h"

using namespace std;


clb_cmd::clb_cmd(const char *str):command(0), ip(0), port(0) {
	Json::Reader reader;
	Json::Value serial;
		
	if (!reader.parse(str, serial, false)) {
		throw "Json string error";
	}
	
	command = serial["command"].asUInt();
	ip = serial["ip"].asUInt();
	port = (unsigned short)serial["port"].asUInt();
	src_groupid = serial["src_groupid"].asUInt();
	dst_groupid = (unsigned short)serial["dst_groupid"].asUInt();
	
	int size = serial["hash_list"].size();
	for (int i=0; i<size; i++) {
		hash_list.push_back(serial["hash_list"][i].asUInt64());
	}
	
	size = serial["group_list"].size();
	for (int i=0; i<size; i++) {
		group_list.push_back(serial["group_list"][i].asUInt());
	}
}

clb_cmd::clb_cmd(const clb_cmd& c) {
	hash_list = c.hash_list;
	group_list = c.group_list;
	command = c.command;
	ip = c.ip;
	port = c.port;
	src_groupid = c.src_groupid;
	dst_groupid = c.dst_groupid;
}


string clb_cmd::serialization(void) {
	Json::Value serial;
	Json::FastWriter writer;
	
	serial["command"] = command;
	serial["ip"] = ip;
	serial["port"] = port;
	
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
	
	return writer.write(serial);
}




/* ==========================================================================================
 *			CLASS: clb_cmd_resp_factory
 * ==========================================================================================
 */
clb_cmd_resp_factory::clb_cmd_resp_factory(const char *str): clb_cmd_resp(0, false) {
	Json::Reader reader;
	Json::Value serial;
		
	if (!reader.parse(str, serial, false)) {
		throw "Json string error";
	}
	
	type = serial["type"].asUInt();
	success = serial["success"].asBool();
	resp = NULL;
	
	try {
		switch(type) {
		case 0:
			resp = new clb_cmd_resp0(str);
			break;		
		case 1:
			resp = new clb_cmd_resp1(str);
			break;		
		case 100:
			resp = new clb_cmd_resp100(str);
			break;		
		}
	}catch(const char *msg) {
		throw msg;
	}
}


clb_cmd_resp_factory::~clb_cmd_resp_factory() {
	if (resp) {
		delete resp;
	}
}


string clb_cmd_resp_factory::serialization(void) {
	if (resp) {
		return resp->serialization();
	}
	return string("");
}


void clb_cmd_resp_factory::dump(void) {
	if (resp) {
		resp->dump();
	}
}



/* ==========================================================================================
 *			CLASS: clb_cmd_resp0
 * ==========================================================================================
 */
clb_cmd_resp0::clb_cmd_resp0(const char *str):clb_cmd_resp(0, false){
	Json::Reader reader;
	Json::Value serial;
		
	if (!reader.parse(str, serial, false)) {
		throw "Json string error";
	}
	
	type = serial["type"].asUInt();
	success = serial["success"].asBool();
	
	int size = serial["resp_list"].size();
	for (int i=0; i<size; i++) {
		CLB_CMD_RESP0 resp = {0};
		
		resp.hash = serial["resp_list"][i]["hash"].asUInt64();
		resp.success = serial["resp_list"][i]["success"].asBool();
		
		Json::Value info = serial["resp_list"][i]["info"];

		resp.info.lb_status = info["lb_status"].asUInt();
		resp.info.stat_status = info["stat_status"].asUInt();
		resp.info.group = info["group"].asUInt();
		resp.info.handle = info["handle"].asInt();

		resp_list.push_back(resp);
	}
}


string clb_cmd_resp0::serialization(void) {
	Json::Value serial;
	Json::FastWriter writer;
		
	serial["type"] = type;
	serial["success"] = success;
	
	list<CLB_CMD_RESP0>::iterator it;
	int idx = 0;
	for (it=resp_list.begin(); it!=resp_list.end(); it++, idx++) {
		CLB_CMD_RESP0 resp = *it;
				
		serial["resp_list"][idx]["hash"] = (Json::UInt64)(resp.hash);
		serial["resp_list"][idx]["success"] = resp.success;
		
		Json::Value info;
		info["lb_status"] = resp.info.lb_status;
		info["stat_status"] = resp.info.stat_status;
		info["group"] = resp.info.group;
		info["handle"] = resp.info.handle;
		serial["resp_list"][idx]["info"] = info;
	}

	return writer.write(serial);
}


void clb_cmd_resp0::dump(void) {
	printf("类型: %d, %s\n", type, success?"操作成功":"操作失败");
		
	if (resp_list.size()) {
		list<CLB_CMD_RESP0>::iterator it;
		printf("HASH			GROUP	STATUS	LB	STAT\n");
		for (it=resp_list.begin(); it!=resp_list.end(); it++) {
			CLB_CMD_RESP0 resp = *it;
	
			printf("%016lx	%d	%s	0x%X	0x%X\n", resp.hash, resp.info.group, 
				resp.success?"OK":"FAIL", resp.info.lb_status, resp.info.stat_status);
			
		}
	}
}




/* ==========================================================================================
 *			CLASS: clb_cmd_resp1
 * ==========================================================================================
 */
clb_cmd_resp1::clb_cmd_resp1(const char *str):clb_cmd_resp(1, false) {
	Json::Reader reader;
	Json::Value serial;
		
	if (!reader.parse(str, serial, false)) {
		throw "Json string error";
	}
	
	type = serial["type"].asUInt();
	success = serial["success"].asBool();
	
	int size = serial["resp_list"].size();
	for (int i=0; i<size; i++) {
		CLB_CMD_RESP1 resp = {0};
		
		resp.group = serial["resp_list"][i]["group"].asUInt();
		resp.success = serial["resp_list"][i]["success"].asBool();
		resp.lb_status = serial["resp_list"][i]["lb_status"].asUInt();
		resp.stat_status = serial["resp_list"][i]["stat_status"].asUInt();
		resp.ip = serial["resp_list"][i]["ip"].asUInt();
		resp.port = (unsigned short)serial["resp_list"][i]["port"].asUInt();
		resp.handle = serial["resp_list"][i]["handle"].asInt();

		resp_list.push_back(resp);
	}
}


string clb_cmd_resp1::serialization(void) {
	Json::Value serial;
	Json::FastWriter writer;
		
	serial["type"] = type;
	serial["success"] = success;
	
	list<CLB_CMD_RESP1>::iterator it;
	int idx = 0;
	for (it=resp_list.begin(); it!=resp_list.end(); it++, idx++) {
		CLB_CMD_RESP1 resp = *it;
				
		serial["resp_list"][idx]["group"] = resp.group;
		serial["resp_list"][idx]["success"] = resp.success;
		serial["resp_list"][idx]["lb_status"] = resp.lb_status;
		serial["resp_list"][idx]["stat_status"] = resp.stat_status;
		serial["resp_list"][idx]["ip"] = resp.ip;
		serial["resp_list"][idx]["port"] = resp.port;
		serial["resp_list"][idx]["handle"] = resp.handle;
	}

	return writer.write(serial);
}


void clb_cmd_resp1::dump(void) {
	printf("类型: %d, %s\n", type, success?"操作成功":"操作失败");
		
	if (resp_list.size()) {
		list<CLB_CMD_RESP1>::iterator it;
		printf("GROUP		STATUS	LB	STAT	HOST\n");
		for (it=resp_list.begin(); it!=resp_list.end(); it++) {
			CLB_CMD_RESP1 resp = *it;
			struct in_addr in;
			in.s_addr = ntohl(resp.ip);
			char *ip_str = inet_ntoa(in);
	
			printf("%08x	%s	0x%X	0x%X	%s:%d\n", resp.group, resp.success?"OK":"FAIL", 
				resp.lb_status, resp.stat_status, ip_str, resp.port);
			
		}
	}
}



/* ==========================================================================================
 *			CLASS: clb_cmd_resp100
 * ==========================================================================================
 */

clb_cmd_resp100::clb_cmd_resp100(const char *str):clb_cmd_resp(100, false) {
	Json::Reader reader;
	Json::Value serial;
		
	if (!reader.parse(str, serial, false)) {
		throw "Json string error";
	}
	
	type = serial["type"].asUInt();
	success = serial["success"].asBool();
	
	int size = serial["resp_list"].size();
	for (int i=0; i<size; i++) {
		CLB_CMD_RESP100 resp = {0};
		
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


string clb_cmd_resp100::serialization(void) {
	Json::Value serial;
	Json::FastWriter writer;
	
	serial["type"] = type;
	serial["success"] = success;
	
	list<CLB_CMD_RESP100>::iterator it;
	int idx = 0;
	for (it=resp_list.begin(); it!=resp_list.end(); it++, idx++) {
		CLB_CMD_RESP100 resp = *it;
				
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
	
	return writer.write(serial);
}


void clb_cmd_resp100::dump(void) {
	printf("类型: %d, %s\n", type, success?"操作成功":"操作失败");
	
	if (resp_list.size()) {
		list<CLB_CMD_RESP100>::iterator it;
		printf("HASH			STATUS	TOTAL	DROPS	TEXTS	ERRORS	TIME\n");
		for (it=resp_list.begin(); it!=resp_list.end(); it++) {
			CLB_CMD_RESP100 resp = *it;
	
			printf("%lx	%s	%ld	%ld	%ld	%ld	%ld\n", resp.hash, resp.success?"OK":"FAIL", 
				resp.info.total, resp.info.drops, resp.info.texts, resp.info.errors, resp.tm.tv_sec);
			
		}
	}
}



