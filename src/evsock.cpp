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


void *evsock::ev_recv(size_t *len) {
	int buf_size = CFG_RCVBUF_SIZE;
	size_t ret_len;
	
	/* ������� */
	if (!len) {
		LOGW("Invalid parameter");
		return NULL;
	}
	
    /* ׼�����ջ����� */
    void *buffer = malloc(buf_size);
    if (buffer == NULL) {
    	LOGE("no memory");
    	*len = -1;
    	return NULL;
    }
    
    /* ����Socket���� */
    ret_len = 0;
    while (1) {
    	/* ��ȡ���� */
	    int rd_size = recv(sockfd, buffer, CFG_RCVBUF_SIZE, MSG_DONTWAIT);
	   	ret_len += rd_size;
	   	if (rd_size <= 0) {
	   		if (errno == EAGAIN) {
	   			break;
	   		}
	   		*len = rd_size;
	   		free(buffer);
	   		return NULL;
	   	} else if (rd_size <= buf_size) {
	   		break;
	   	}
	   	
	   	/* ���������㣬�������� */
	   	buf_size *= 2;
	   	buffer = realloc(buffer, buf_size);
	    if (buffer == NULL) {
	    	LOGE("no memory");
	    	*len = -1;
	    	return NULL;
	    }
	}
	*len = ret_len;
   	return buffer;
}


void evsock::recv_done(void *buf) {
	free(buf);
}


void evsock::do_write(int sock, short event, void* arg) {
	evsock *evsk = (evsock *)arg;
	queue<EV_SEND*> *q = evsk->ev_queue();
	
    /* ��д�������ȡ��һ��Buffer */
    EV_SEND *ps = q->front();
    q->pop();
    
    /* �������� */
	ps->len = send(sock, ps->buf, ps->len, 0);
	
	/* ֪ͨ������� */
	evsk->send_done(ps->token, ps->buf, ps->len);
	
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


bool evsock::ev_send(unsigned long int token, void *buf, size_t size) {
	bool empty;
	
	if (!buf || !size) {
		return false;
	}
		
	/* �ѵ�ǰ����������дFIFO */
	EV_SEND *send = new EV_SEND(token, buf, size);
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

