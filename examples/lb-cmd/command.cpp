﻿
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include "command.h"

#define CFG_LB_IPADDR		"127.0.0.1"
#define CFG_LB_CMDPORT		10000

command::command() {
	struct sockaddr_in serv_addr;
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	/* 设置连接目的地址 */
	memset(&serv_addr, 0, sizeof(serv_addr));
 	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(CFG_LB_CMDPORT);
	serv_addr.sin_addr.s_addr = inet_addr(CFG_LB_IPADDR);

	/* 发送连接请求 */
	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) < 0) {
		throw "connect failed";
	}
}	

command::~command() {
	if (sockfd > 0) {
		close(sockfd);
	}
}

int command::do_request(void *buf, size_t len) {
	
	/* 参数检查 */
	if ((sockfd < 0) || !buf || (len < 0)) {
		return -1;
	}
	
	if (!len) {
		return len;
	}

	/* 发送请求数据 */
	return send(sockfd, buf, len, 0);
}


int command::get_repsone(void *buf, int *size) {
	void *pwrite;
	
	/* 确认连接已经建立 */
	if ((sockfd < 0) || !buf || !size) {
		return -1;
	}
	
	/* 等待数据 */
	pwrite = buf;
	while(1) {
		fd_set fdread;
		FD_ZERO(&fdread);  
		FD_SET(sockfd, &fdread); 
		struct timeval tv = {3, 0};   
		int ret = select(0, &fdread, NULL, NULL, &tv); 
		if (ret == 0) 
		{       
			/* Time expired */
		    break; 
		}
		printf("abc\n");
		if (FD_ISSET(sockfd, &fdread)) {
			int len = *size;
			len = recv(sockfd, buf, len, 0); 
			return len;
		}
	}
	return 0;
}


int command::request(unsigned int cmd, unsigned long int hash, unsigned int id,
					unsigned int ip, unsigned int port) {
	LB_CMD req;
	
	/* 下发命令到服务器 */	
	req.command = cmd;
	req.group = id;
	req.hash = hash;
	req.ip = ip;
	req.port = port;
	if (do_request(&req, sizeof(req)) != sizeof(req)) {
		printf("连接服务器失败\n");
		return -1;
	}
	
	return 0;
}

int command::reponse(void) {
	char repbuf[1024];
	int len;
	
	/* 获取服务器应答 */
	len = 1024;
	len = get_repsone(repbuf, &len);
	if (len <= 0) {
		printf("服务器返回失败\n");
		return -1;
	}
	
	/* 处理服务器应答 */
	if (len >= sizeof(bool)) {
		int ret = *(bool*)repbuf;
		if (ret == false) {
			printf("处理失败\n");
			return -1;
		} 
		printf("处理成功\n");
	}
}
