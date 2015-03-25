#ifndef __EV_CLNT_H__
#define __EV_CLNT_H__

#include <cstdlib>
#include <cstddef>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <event.h>
#include <pthread.h>
#include <errno.h>

#include <config.h>
#include <utils/log.h>
#include <evobj.h>
#include <evsock.h>

using namespace std;	

template<typename T> 
class evclnt : public evobj {
public:
	~evclnt();
	evclnt(struct in_addr ip, unsigned short port, struct event_base* eb=NULL);
	evclnt(const char *ip_str, unsigned short port, struct event_base* eb=NULL);
	int setskopt(int level, int optname,
                      const void *optval, socklen_t optlen);
	int getskopt(int level, int optname,
                      void *optval, socklen_t *optlen);
	T *evconnect(void);
	T *get_evsock(void);
	void ev_close(void);
	bool loop(void);
	bool loop_thread(void);
	
protected:
	
private:
	int sockfd;
	struct in_addr ip_addr;
	unsigned short port;
	pthread_t th_clnt;
	struct event_base* base;
	T *evsk_dr;
	bool balloc_base;
	
	static void *clnt_worker_thread(void *args);
};


template<class T>
evclnt<T>::~evclnt() {
	if (evsk_dr) {
		delete evsk_dr;
	}
	if (balloc_base) {
		event_base_free(base);
	}
}


template<class T>
evclnt<T>::evclnt(struct in_addr ip, unsigned short prt, struct event_base* eb) {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
    	throw "socket error";
    }
    ip_addr = ip;
    port = prt;
    evsk_dr = NULL;
    base = eb;
    th_clnt = 0;
   	balloc_base = false;
    
    int on = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&on, sizeof(on));  
}


template<class T>
evclnt<T>::evclnt(const char *ipstr, unsigned short prt, struct event_base* eb) {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
    	throw "socket error";
    }
    ip_addr.s_addr = inet_addr(ipstr);
    port = prt;
    evsk_dr = NULL;
    base = eb;
    th_clnt = 0;
   	balloc_base = false;
    
    int on = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&on, sizeof(on));  
}

template<class T>
void evclnt<T>::ev_close(void) {
	if (balloc_base) {
		event_base_loopbreak(base);
	} else {
		delete this;
	}
}


template<class T>
int evclnt<T>::setskopt(int level, int optname,
                      const void *optval, socklen_t optlen) {
    return setsockopt(sockfd, level, optname, &optval, optlen);
}


template<class T>
int evclnt<T>::getskopt(int level, int optname,
                      void *optval, socklen_t *optlen) {
    return getsockopt(sockfd, level, optname, &optval, optlen);
}


template<class T>
T *evclnt<T>::get_evsock(void) {
	return evsk_dr;
}


template<class T>
T *evclnt<T>::evconnect(void) {
	
	/* 监听地址 */
	struct sockaddr_in sk_addr;
	memset(&sk_addr, 0, sizeof(sk_addr));
	sk_addr.sin_family = AF_INET;
	sk_addr.sin_port = htons(port);
	sk_addr.sin_addr = ip_addr;
	
	if (connect(sockfd, (struct sockaddr*)&sk_addr, sizeof(struct sockaddr)) < 0) {
		LOGE ("connect to %d error: %s", port, strerror(errno));
		return NULL;
	}
	
	/* 等待客户端接入 */
	if (base == NULL) {
		base = event_base_new();
		if (base == NULL) {
			LOGE ("event_base_new error");
			return NULL;
		}
	   	balloc_base = true;
	}
	
    /* 创建evsock对象 */
	evsk_dr = new T(sockfd, base);
	evsock *evsk = dynamic_cast<evsock *>(evsk_dr);
	
	/* 创建evsock事件 */
	event_set(&evsk->read_ev, sockfd, EV_READ|EV_PERSIST, evsk_dr->read, evsk_dr);
	if (event_base_set(base, &evsk->read_ev) < 0) {
		LOGE("event_base_set error\n");
		delete evsk_dr;
		evsk_dr = NULL;
		return NULL;
	}
	if (event_add(&evsk->read_ev, NULL) < 0) {
		LOGE("event_add error\n");
		delete evsk_dr;
		evsk_dr = NULL;
		return NULL;
	}
	
	/* 捆绑evsock和evclnt */
	evsk_dr->evobj_bind(this);
	
	return evsk_dr;
}


template<class T>
bool evclnt<T>::loop(void) {
	
	/* 事件处理循环 */
	event_base_dispatch(base);
	return true;
}


template<class T>
void *evclnt<T>::clnt_worker_thread(void *args) {
	evclnt<T> *pclnt = (evclnt<T> *)args;
	
	pclnt->loop();
	
	return NULL;
}


template<class T>
bool evclnt<T>::loop_thread(void) {
	
	if (pthread_create(&th_clnt, NULL, evclnt<T>::clnt_worker_thread, this) != 0) {
		LOGE("Create worker thread failed");
		return false;
	}
	return true;
}

#endif /* __EV_CLNT_H__ */ 
