#ifndef __EVSOCK_H__
#define __EVSOCK_H__

#include <cstdlib>
#include <cstddef>
#include <event.h>
#include <config.h>
#include <queue>

using namespace std;

class evsock {
private:
	class EV_SEND {
	public:
		unsigned long int token;
		void *buf;
		size_t len;
		
	public:
		~EV_SEND() {};
		EV_SEND(unsigned long int token, void *buf, size_t len): token(token), buf(buf), len(len) {};
			
	protected:		
	private:
	};
		
public:
	virtual ~evsock();
	evsock(int fd, struct event_base* base): sockfd(fd), evbase(base) {};
    struct event read_ev;
    struct event write_ev;
	
	void *ev_recv(size_t *len);
	void recv_done(void *buf);
	
	bool ev_send(unsigned long int token, void *buf, size_t len);
	virtual void send_done(unsigned long int token, void *buf, size_t len) = 0;

	queue<EV_SEND*> *ev_queue(void) {return &wq;}
	
	static void do_write(int sock, short event, void* arg);

protected:
	
private:
	int sockfd;
	struct event_base* evbase;
	queue<EV_SEND*> wq;
};
	
#endif /* __EVSOCK_H__ */ 