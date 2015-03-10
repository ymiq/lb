#ifndef __EVSOCK_H__
#define __EVSOCK_H__

#include <cstdlib>
#include <cstddef>
#include <event.h>
#include <config.h>
#include <queue>
#include <qao/qao_base.h>

using namespace std;

class evsock {
private:
	class EV_SEND {
	public:
		~EV_SEND() {};
		EV_SEND(const void *buf, size_t len, qao_base *qao) 
									: buf(buf), len(len), qao(qao) {};
			
	public:
		const void *buf;
		size_t len;		
		qao_base *qao;
	};
		
public:
	virtual ~evsock();
	evsock(int fd, struct event_base* base): sockfd(fd), evbase(base) {};
    struct event read_ev;
    struct event write_ev;
	
	void *ev_recv(size_t &len);
	void recv_done(void *buf);
	
	bool ev_send(const void *buf, size_t len);
	bool ev_send(qao_base *qao);
	virtual void send_done(void *buf, size_t len, bool send_ok) = 0;
	virtual void send_done(qao_base *qao, bool send_ok) = 0;

	queue<EV_SEND*> *ev_queue(void) {return &wq;}
	
	static void do_write(int sock, short event, void* arg);

protected:
	
private:
	int sockfd;
	struct event_base* evbase;
	queue<EV_SEND*> wq;
};
	
#endif /* __EVSOCK_H__ */ 