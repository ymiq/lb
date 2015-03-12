﻿#include <cstdio>
#include <cstring>
#include <sys/types.h>
#include <unistd.h>
#include <log.h>
#include <qao/qao_base.h>

using namespace std;

unsigned int qao_base::seqno = 0;


int qao_base::set_qos(int qos) {
	int ret = qao_qos; 
	qao_qos = qos & 0x3; 
	return ret;
}


int qao_base::get_qos(void) {
	return qao_qos;
}


void qao_base::init(int type, int version, int qos) {
	pid_t pid = getpid();
	unsigned long token = (unsigned long)pid;
	token <<= 32;
	token |= __sync_fetch_and_add(&qao_base::seqno, 1);
	qao_token = token;
	qao_type = ((unsigned int) type) & 0xff;
	qao_version = ((unsigned int)version) & 0x3f;
	qao_qos = ((unsigned int)qos) & 0x03;
}


void qao_base::init(serial_data *header) {
	qao_token = header->token;
	qao_type = header->type;
	qao_version = header->version & 0x3f;
	qao_qos = (header->version & 0xc0) >> 6;
}


void qao_base::serial_header(serial_data *header) {
	header->token = qao_token;
	header->type = qao_type;
	header->version = (qao_version & 0x3f) | ((qao_qos & 0x3) << 6);
}


void qao_base::serial_header(serial_data *header, int len_off, int datalen, int fragment) {
	header->length = ((unsigned int) len_off) & 0x03ffffff;
	header->length |= (((unsigned int) fragment) & 0x03) << 30;
	header->datalen = (unsigned short)datalen;
	serial_header(header);
}


void qao_base::serial_header(serial_data *header, int len_off, int datalen) {
	serial_header(header, len_off, datalen, 0);
}
