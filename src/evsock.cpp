#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <event.h>
#include <log.h>
#include <evsock.h>

#define CFG_RCVBUF_SIZE		1024

evsock::~evsock() {
    event_del(&read_ev);
	if (!wq.empty()) {
	    event_del(&write_ev);
	}
}


void *evsock::ev_recv(size_t *size) {
	/* ������� */
	if (!size) {
		LOGE("Invalid parameter");
		return NULL;
	}
	
    /* ׼�����ջ����� */
    char *buffer = (char*)malloc(CFG_RCVBUF_SIZE);
    if (buffer == NULL) {
    	LOGE("no memory");
    	*size = -1;
    	return NULL;
    }
    
    /* ����Socket���� */
    int rd_size = recv(sockfd, buffer, CFG_RCVBUF_SIZE, 0);
    *size = rd_size;
   	if (rd_size <= 0) {
   		free(buffer);
   		return NULL;
   	} 
   	return buffer;
}


void evsock::ev_recv_done(void *buf) {
	free(buf);
}


void evsock::do_write(int sock, short event, void* arg) {
	evsock *evsk = (evsock *)arg;
	queue<EV_SEND*> *q = evsk->ev_queue();
	
    /* ��д�������ȡ��һ��Buffer */
    EV_SEND *qsend = q->front();
    q->pop();
    
    /* ���� */
	qsend->size = send(sock, qsend->buf, qsend->size, 0);
	evsk->ev_send_done(qsend);
    
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


bool evsock::ev_send(EV_SEND *send) {
	bool empty;
	
	if (!send->buf || !send->size) {
		return false;
	}
		
	/* �ѵ�ǰ����������дFIFO */
	EV_SEND *qsend = (EV_SEND *)malloc(sizeof(EV_SEND));
	*qsend = *send;
	empty = wq.empty();
	wq.push(qsend);
	
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

