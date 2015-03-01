#ifndef __EVSOCK_H__
#define __EVSOCK_H__

#include <cstdlib>
#include <cstddef>
#include <event.h>
#include <config.h>
#include <queue>

using namespace std;

typedef struct EV_SEND {
	unsigned long int token;
	void *buf;
	size_t size;
}EV_SEND;

class evsock {
public:
	~evsock();
	evsock(int fd, struct event_base* base): sockfd(fd), evbase(base) {};
    struct event read_ev;
    struct event write_ev;
	
	void *ev_recv(size_t *size);
	void ev_recv_done(void *buf);
	
	bool ev_send(EV_SEND *send);
	virtual void ev_send_done(EV_SEND *send) = 0;

	queue<EV_SEND*> *ev_queue(void) {return &wq;}
	
	static void do_write(int sock, short event, void* arg);
		
protected:
	
private:
	int sockfd;
	struct event_base* evbase;
	queue<EV_SEND*> wq;
};
	
#endif /* __EVSOCK_H__ */ 