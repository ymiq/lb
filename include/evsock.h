#ifndef __EVSOCK_H__
#define __EVSOCK_H__

#include <cstdlib>
#include <cstddef>
#include <event.h>
#include <config.h>
#include <job_queue.h>
#include <qao/qao_base.h>
#include <hash_tbl.h>
#include <evobj.h>

using namespace std;

/* �������ݰ���Ƭ��С */
#define CFG_SECTION_SIZE		(16*1024)

class evsock {
private:
	class ev_job {
	public:
		~ev_job() {};
		ev_job(char *buf, size_t len, qao_base *qao) 
						: buf(buf), len(len), qao(qao), section(false) {};
			
	public:
		char *buf;
		size_t len;		
		qao_base *qao;
		bool section;				/* �ֶΣ�һ������������Ӧ�ò㱻��ֳɴ�С������SECTION_SIZE�����ݶν��з��� */
		unsigned int offset;
		serial_data header;
	};
		
public:
	virtual ~evsock();
	evsock(int fd, struct event_base* base);
    struct event read_ev;
    struct event write_ev;
    struct event thw_ev;
	
	void ev_close(void);
	void evobj_bind(evobj *pobj);
	
	void *ev_recv(size_t &len, bool &partition);
	void *ev_recv_raw(size_t &len);
	void recv_done(void *buf);
	
	bool ev_send(const void *buf, size_t len);
	bool ev_send(const void *buf, size_t len, int qos);
	bool ev_send(qao_base *qao);
	
	bool ev_send_inter_thread(const void *buf, size_t len);
	bool ev_send_inter_thread(const void *buf, size_t len, int qos);
	bool ev_send_inter_thread(qao_base *qao);
	
	virtual void send_done(void *buf, size_t len, bool send_ok) = 0;
	virtual void send_done(qao_base *qao, bool send_ok) = 0;

	job_queue<ev_job*> *ev_queue(void) {return &wq;}
	
	static void do_write(int sock, short event, void* arg);
	static void do_eventfd(int efd, short event, void* arg);

protected:
	int sockfd;
	struct event_base* evbase;
	evobj *pevobj;
	
private:
	/* ��Ƭ������ر��� */
	bool frag_flag;				/* ��Ƭ: һ���������ݶα�Э��ջ��ֳɶ����Ƭ */
	bool frag_sec;				/* ���ڷ�Ƭ����Ϊ�ֶα��ķ�Ƭ */
	char *frag_buf;
	size_t frag_len;			/* �������ܳ��� */
	size_t frag_off;			/* ����������ƫ�� */
	serial_data frag_header;
	
	/* �ֶν�����ر��� */
	unsigned long sec_token;	
	char *sec_buf;				/* ���ݻ����� */
	unsigned int sec_len;		/* �������ܳ��� */
	unsigned int sec_off;		/* ����������ƫ�� */
	
	/* ���߳�д�¼� */
	int efd;
	job_queue<ev_job*> wq;
	
	void frag_prepare(void *buf, size_t len, size_t off, bool bsec);
	int ev_recv_frag(size_t &len, bool &partition);
};
	
#endif /* __EVSOCK_H__ */ 