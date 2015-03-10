#include <cstring>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <event.h>
#include <log.h>
#include <evsock.h>

#define CFG_RCVBUF_SIZE		1024

evsock::~evsock() {
	/* ɾ�����¼� */
    event_del(&read_ev);
    
    /* ɾ��д�¼� */
	if (!wq.empty()) {
	    event_del(&write_ev);
	}
	
	/* �ͷŶ��� */
	queue<EV_SEND*> *q = this->ev_queue();
	while (!wq.empty()) {
	    EV_SEND *ps = q->front();
	    q->pop();
	    
		/* �ͷ�EV_SEND */
		delete ps;
	}
}

/* ��δ�������ݰ���Ƭ����! */
void *evsock::ev_recv(size_t &len) {
	size_t ret_len;
	
	/* ����ͷ��Ϣ */
	serial_data header;
    int rd_size = recv(sockfd, &header, sizeof(serial_data), MSG_DONTWAIT);
    if (rd_size != sizeof(serial_data)) {
    	if (rd_size <= 0) {
    		len = rd_size;
    	} else {
   			len = -1;
   		}
    	return NULL;
    }
    if (header.length <= sizeof(serial_data)) {
    	LOGE("recv header failed, lenght: %d", header.length);
    	len = -1;
    	return NULL;
    }
    ret_len = header.length - sizeof(serial_data);
	
    /* ׼�����ջ����� */
    char *buffer = new char[header.length & 0x3fffffffu];
    memcpy(buffer, &header, sizeof(serial_data));
    char *pwrite = buffer + sizeof(serial_data);
    rd_size = recv(sockfd, pwrite, ret_len, MSG_DONTWAIT);
    if (rd_size != (int)ret_len) {
    	LOGE("recv length umatch, want %d but %d", ret_len, rd_size);
    	delete buffer;
    	len = -1;
    	return NULL;
    }
    
	len = header.length & 0x3fffffffu;
   	return buffer;
}


void evsock::recv_done(void *buf) {
	delete (char *)buf;
}


void evsock::do_write(int sock, short event, void* arg) {
	evsock *evsk = (evsock *)arg;
	queue<EV_SEND*> *q = evsk->ev_queue();
	
    /* ��д�������ȡ��һ��Buffer */
    EV_SEND *ps = q->front();
    q->pop();
    
    /* �������� */
    if (ps->qao) {
		bool send_ok = false;
		size_t wlen;
		
		/* ���л����󲢷��� */
		char *buf = (char*)ps->qao->serialization(wlen);
		if (buf) {
			int len = send(sock, buf, wlen, 0);
			if (len == (int)wlen) {
				send_ok = true;
			}
			delete buf;
		}
		
		/* ֪ͨ������� */
		evsk->send_done(ps->qao, send_ok);
    } else {
		bool send_ok = true;
		
		/* ���ͻ����������� */
		int len = send(sock, ps->buf, ps->len, 0);
		if (len == (int)ps->len) {
			send_ok = true;
		}
		
		/* ֪ͨ������� */
		evsk->send_done((void*)ps->buf, ps->len, send_ok);
	}
	
	/* �ͷ�EV_SEND */
	delete ps;
    
    /* ���FIFO�Ƿ񻹴��ڴ��������� */
    if (!q->empty()) {
	    event_set(&evsk->write_ev, sock, EV_WRITE, do_write, evsk);
	    if (event_base_set(evsk->evbase, &evsk->write_ev) < 0) {
	    	LOGE("event_base_set error\n");
	    	return;
	    }
	    if (event_add(&evsk->write_ev, NULL) < 0) {
	    	LOGE("event_add error\n");
	    }
    }
}


bool evsock::ev_send(const void *buf, size_t size) {
	bool empty;
	
	if (!buf || !size) {
		return false;
	}
		
	/* �ѵ�ǰ����������дFIFO */
	EV_SEND *send = new EV_SEND(buf, size, NULL);
	empty = wq.empty();
	wq.push(send);
	
	/* ����д�¼� */
	if (empty) {
	    event_set(&write_ev, sockfd, EV_WRITE, do_write, this);
	    if (event_base_set(evbase, &write_ev) < 0) {
	    	LOGE("event_base_set error\n");
	    	return false;
	    }
	    if (event_add(&write_ev, NULL) < 0) {
	    	LOGE("event_add error\n");
	    	return false;
	    }
	}
    return true;
}



bool evsock::ev_send(qao_base *qao) {
	bool empty;
	
	if (!qao) {
		return false;
	}
		
	/* �ѵ�ǰ����������дFIFO */
	EV_SEND *send = new EV_SEND(NULL, 0, qao);
	empty = wq.empty();
	wq.push(send);
	
	/* ����д�¼� */
	if (empty) {
	    event_set(&write_ev, sockfd, EV_WRITE, do_write, this);
	    if (event_base_set(evbase, &write_ev) < 0) {
	    	LOGE("event_base_set error\n");
	    	return false;
	    }
	    if (event_add(&write_ev, NULL) < 0) {
	    	LOGE("event_add error\n");
	    	return false;
	    }
	}
    return true;
}

