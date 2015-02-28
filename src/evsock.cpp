#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <event.h>
#include <log.h>

evsock::evsock() {
}


evsock::~evsock() {
}


evsock::evsock(int fd) {
	sockfd = fd;
}


virtual void *evsock::read(size_t *buf_size) {
    /* 准备接收缓冲区 */
    char *buffer = (char*)malloc(MEM_SIZE);
    if (buffer == NULL) {
    	LOGE("no memory\n");
    	return;
    }
    bzero(buffer, MEM_SIZE);
    
    /* 接收Socket数据 */
    size = recv(sock, buffer, MEM_SIZE, 0);
    if (size <= 0) {
    	if (size == 0) {
    		/* 客户端断开连接，在这里移除读事件并且释放客户数据结构 */
    	} else {
			/* 出现了其它的错误，在这里关闭socket，移除事件并且释放客户数据结构 */
    	}
        release_sock_event(ev);
        close(sock);
        return;
    }
    return buffer;
}


void evsock::free_buff(void *buf) {
	free(buffer);
}


virtual int evsock::write(void *buffer, size_t size) {
    retrun send(sock, buffer, size, 0);
}

