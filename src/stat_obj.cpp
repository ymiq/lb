#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <stat_obj.h>
#include <rcu_obj.h>
#include <rcu_man.h>
#include <stat_man.h>

#include <exception>

using namespace std; 

stat_obj::stat_obj() {
	flags = 0;
	memset(&info, 0, sizeof(info));
}


stat_obj::~stat_obj() {
}


int stat_obj::start(unsigned int code) {
	flags |= code;
	return 0;
}

int stat_obj::stop(unsigned int code) {
	flags &= ~code;
	return 0;
}

int stat_obj::clear(unsigned int code) {
	flags &= ~code;
	memset(&info, 0, sizeof(info));
	return 0;
}


int stat_obj::read(stat_info *pinfo) {
	*pinfo = info;
	return 0;
}


int stat_obj::stat(void *packet, int packet_size) {
	info.total_packets++;
	return 0;
}


int stat_obj::stat() {
	info.total_packets++;
	return 0;
}


int stat_obj::error_stat() {
	info.error_packets++;
	return 0;
}


stat_obj& stat_obj::operator+=(const stat_obj *pobj) {	
	this->info.total_packets += pobj->info.total_packets;
	this->info.error_packets += pobj->info.error_packets;	
	return *this;
}


stat_obj& stat_obj::operator+=(const stat_obj &obj) {	
	this->info.total_packets += obj.info.total_packets;
	this->info.error_packets += obj.info.error_packets;	
	return *this;
}
