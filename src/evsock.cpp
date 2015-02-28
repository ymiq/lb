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
    /* ׼�����ջ����� */
    char *buffer = (char*)malloc(MEM_SIZE);
    if (buffer == NULL) {
    	LOGE("no memory\n");
    	return;
    }
    bzero(buffer, MEM_SIZE);
    
    /* ����Socket���� */
    size = recv(sock, buffer, MEM_SIZE, 0);
    if (size <= 0) {
    	if (size == 0) {
    		/* �ͻ��˶Ͽ����ӣ��������Ƴ����¼������ͷſͻ����ݽṹ */
    	} else {
			/* �����������Ĵ���������ر�socket���Ƴ��¼������ͷſͻ����ݽṹ */
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

