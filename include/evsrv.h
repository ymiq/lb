#ifndef __EVSRV_H__
#define __EVSRV_H__

#include <cstdlib>
#include <cstddef>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <event.h>

#include <config.h>
#include <utils/log.h>
#include <evsock.h>

using namespace std;	
#define CFG_LISTEN_BACKLOG     4096

template<typename T> 
class evsrv {
public:
	~evsrv();
	evsrv(struct in_addr ip, unsigned short port);
	evsrv(const char *ip_str, unsigned short port);
	int setskopt(int level, int optname,
                      const void *optval, socklen_t optlen);
	int getskopt(int level, int optname,
                      void *optval, socklen_t *optlen);
	bool loop(void);
	
	
protected:
	
private:
	int sockfd;
	struct in_addr ip_addr;
	unsigned short port;
	struct event_base* base;
	
	static void do_accept(int sock, short event, void* arg);	
};

template<class T>
evsrv<T>::~evsrv() {
	close(sockfd);
}


template<class T>
evsrv<T>::evsrv(struct in_addr ip, unsigned short prt) {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
    	throw "socket error";
    }
    ip_addr = ip;
    port = prt;
}


template<class T>
evsrv<T>::evsrv(const char *ipstr, unsigned short prt) {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
    	throw "socket error";
    }
    ip_addr.s_addr = inet_addr(ipstr);
    port = prt;
}


template<class T>
int evsrv<T>::setskopt(int level, int optname,
                      const void *optval, socklen_t optlen) {
    return setsockopt(sockfd, level, optname, &optval, optlen);
}


template<class T>
int evsrv<T>::getskopt(int level, int optname,
                      void *optval, socklen_t *optlen) {
    return getsockopt(sockfd, level, optname, &optval, optlen);
}

                     
template<class T>
bool evsrv<T>::loop(void) {
	struct sockaddr_in sk_addr;
	
	/* 监听地址 */
	memset(&sk_addr, 0, sizeof(sk_addr));
	sk_addr.sin_family = AF_INET;
	sk_addr.sin_port = htons(port);
	sk_addr.sin_addr = ip_addr;
	
	if (bind(sockfd, (struct sockaddr*)&sk_addr, sizeof(struct sockaddr)) < 0) {
		LOGE("bind error");
		return false;
	}
	if (listen(sockfd, CFG_LISTEN_BACKLOG) < 0) {
		LOGE("listen error");
		return false;
	}
	
	/* 设置句柄为非阻塞 */
	evutil_make_socket_nonblocking(sockfd);
	
	/* 等待客户端接入 */
	struct event listen_ev;
	base = event_base_new();
	if (base == NULL) {
		LOGE("event_base_new error\n");
		return false;
	}
	event_set(&listen_ev, sockfd, EV_READ|EV_PERSIST, evsrv<T>::do_accept, this);
	if (event_base_set(base, &listen_ev) < 0) {
		LOGE("event_base_set error\n");
		return false;
	}
	if (event_add(&listen_ev, NULL) < 0) {
		LOGE("event_base_set error\n");
		return false;
	}
	
	/* 事件处理循环 */
	event_base_dispatch(base);
	
	/* Never reache here */
	return true;
}


template<class T>
void evsrv<T>::do_accept(int sock, short event, void* arg) {
	struct sockaddr_in cli_addr;
	evsrv<T> *srv = (evsrv<T> *)arg;
	int newfd;
	socklen_t sin_size;
	
    /* 接受连接 */
	sin_size = sizeof(struct sockaddr_in);
	newfd = accept(srv->sockfd, (struct sockaddr*)&cli_addr, &sin_size);
	if (newfd < 0) {
		LOGE("accept error");
		return;
	}
	
    /* 创建evsock对象 */
	T *evsk_dr = new T(newfd, srv->base);
	evsock *evsk = dynamic_cast<evsock *>(evsk_dr);
	
	/* 创建evsock事件 */
	event_set(&evsk->read_ev, newfd, EV_READ|EV_PERSIST, evsk_dr->read, evsk_dr);
	if (event_base_set(srv->base, &evsk->read_ev) < 0) {
		LOGE("event_base_set error\n");
		return;
	}
	if (event_add(&evsk->read_ev, NULL) < 0) {
		LOGE("event_add error\n");
		return;
	}
}

#endif /* __EVSRV_H__ */ 
