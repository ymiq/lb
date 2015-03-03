
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include "cmd_clnt.h"

#define CFG_CLB_IPADDR		"127.0.0.1"
#define CFG_CLB_CMDPORT		8000
#define CFG_RCVBUF_SIZE		2048

cmd_clnt::cmd_clnt() {
	struct sockaddr_in serv_addr;
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	/* 设置连接目的地址 */
	memset(&serv_addr, 0, sizeof(serv_addr));
 	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(CFG_CLB_CMDPORT);
	serv_addr.sin_addr.s_addr = inet_addr(CFG_CLB_IPADDR);

	/* 发送连接请求 */
	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) < 0) {
		throw "connect failed";
	}
}	

cmd_clnt::~cmd_clnt() {
	if (sockfd > 0) {
		close(sockfd);
	}
}

int cmd_clnt::do_request(const void *buf, size_t len) {
	
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


void *cmd_clnt::do_recv(size_t *len) {
	int buf_size = CFG_RCVBUF_SIZE;
	size_t ret_len;
	
    /* 准备接收缓冲区 */
    void *buffer = malloc(buf_size);
    if (buffer == NULL) {
    	printf("内存不足，申请大小: %d", buf_size);
    	*len = -1;
    	return NULL;
    }
    
    /* 接收Socket数据 */
    ret_len = 0;
    while (1) {
    	/* 读取数据 */
	    int rd_size = recv(sockfd, buffer, CFG_RCVBUF_SIZE, MSG_DONTWAIT);
	   	ret_len += rd_size;
	   	if (rd_size <= 0) {
	   		if (errno == EAGAIN) {
	   			break;
	   		}
	   		*len = rd_size;
	   		free(buffer);
	   		return NULL;
	   	} else if (rd_size <= buf_size) {
	   		break;
	   	}
	   	
	   	/* 缓冲区不足，重新申请 */
	   	buf_size *= 2;
	   	buffer = realloc(buffer, buf_size);
	    if (buffer == NULL) {
	    	printf("内存不足，申请大小: %d", buf_size);
	    	*len = -1;
	    	return NULL;
	    }
	}
	*len = ret_len;
   	return buffer;
}


void *cmd_clnt::get_repsone(size_t *len) {
	
	/* 确认连接已经建立 */
	if ((sockfd < 0) || !len) {
		return NULL;
	}
	
	/* 等待数据 */
	while(1) {
		fd_set fdread;
		FD_ZERO(&fdread);  
		FD_SET(sockfd, &fdread); 
		struct timeval tv = {3, 0};   
		int ret = select(sockfd+1, &fdread, NULL, NULL, &tv); 
		if (ret == 0) 
		{       
			/* Time expired */
		    break; 
		}
		if (FD_ISSET(sockfd, &fdread)) {
			return do_recv(len);
		}
	}
	return NULL;
}


int cmd_clnt::request(clb_cmd &cmd) {
	string str = cmd.serialization();
	int len = str.length();
	
	if (len > 0) {
		len += 1;
		if (do_request(str.c_str(), len) != len) {
			printf("连接服务器失败\n");
			return -1;
		}
	}
	return 0;
}

int cmd_clnt::reponse(void) {
	void *resp_buf;
	size_t len;
	
	/* 获取服务器应答 */
	resp_buf = get_repsone(&len);
	if (resp_buf == NULL) {
		printf("服务器返回失败\n");
		return -1;
	}
	
	/* 处理服务器应答 */
	try {
		clb_cmd_resp_factory resp((const char*)resp_buf);
		resp.dump();
	} catch(const char *msg) {
		printf("服务器返回数据错误\n");
	}
	
	/* 释放缓冲区 */
	free(resp_buf);
	return 0;
}
