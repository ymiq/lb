#ifndef __EVSOCK_H__
#define __EVSOCK_H__

#include <cstdlib>
#include <cstddef>
#include <event.h>
#include <config.h>
#include <pri_queue.h>
#include <qao/qao_base.h>
#include <hash_tbl.h>

using namespace std;

/* 定义数据包分片大小 */
#define CFG_FRAGMENT_SIZE		(16*1024)

typedef struct frag_pack {
	unsigned long token;	
	unsigned long time;		/* 时间戳 */
	unsigned int length;	/* 数据包总长度 */
	unsigned int rcvlen;	/* 接收总长度 */
	char *buffer;			/* 数据缓冲区 */
}frag_pack;

class evsock {
private:
	class ev_job {
	public:
		~ev_job() {};
		ev_job(char *buf, size_t len, qao_base *qao) 
						: buf(buf), len(len), qao(qao), fragment(false) {};
			
	public:
		char *buf;
		size_t len;		
		qao_base *qao;
		bool fragment;
		unsigned int offset;
		serial_data header;
	};
		
public:
	virtual ~evsock();
	evsock(int fd, struct event_base* base);
    struct event read_ev;
    struct event write_ev;
    struct event thw_ev;
	
	void quit(void);
	
	void *ev_recv(size_t &len, bool &fragment);
	void recv_done(void *buf);
	
	bool ev_send(const void *buf, size_t len);
	bool ev_send(const void *buf, size_t len, int qos);
	bool ev_send(qao_base *qao);
	
	bool ev_send_inter_thread(const void *buf, size_t len);
	bool ev_send_inter_thread(const void *buf, size_t len, int qos);
	bool ev_send_inter_thread(qao_base *qao);

	virtual void send_done(void *buf, size_t len, bool send_ok) = 0;
	virtual void send_done(qao_base *qao, bool send_ok) = 0;

	pri_queue<ev_job*> *ev_queue(void) {return &wq;}
	
	static void do_write(int sock, short event, void* arg);
	static void do_eventfd(int efd, short event, void* arg);

protected:
	int sockfd;
	struct event_base* evbase;
	
private:
	int efd;
	pri_queue<ev_job*> wq;
	hash_tbl<frag_pack, 256> frags;
};
	
#endif /* __EVSOCK_H__ */ 