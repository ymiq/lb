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
	/* 删除读事件 */
    event_del(&read_ev);
    
    /* 删除写事件 */
	if (!wq.empty()) {
	    event_del(&write_ev);
	}
	
	/* 释放对象 */
	queue<EV_SEND*> *q = this->ev_queue();
	while (!wq.empty()) {
	    EV_SEND *ps = q->front();
	    q->pop();
	    
		/* 释放EV_SEND */
		delete ps;
	}
}


void *evsock::ev_recv(size_t *len) {
	int buf_size = CFG_RCVBUF_SIZE;
	size_t ret_len;
	
	/* 参数检查 */
	if (!len) {
		LOGW("Invalid parameter");
		return NULL;
	}
	
    /* 准备接收缓冲区 */
    void *buffer = malloc(buf_size);
    if (buffer == NULL) {
    	LOGE("no memory");
    	*len = -1;
    	return NULL;
    }
    
    /* 接收Socket数据 */
    ret_len = 0;
    while (1) {
    	/* 读取数据 */
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
	   	
	   	/* 缓冲区不足，重新申请 */
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
	
    /* 从写缓冲队列取出一个Buffer */
    EV_SEND *ps = q->front();
    q->pop();
    
    /* 发送数据 */
	ps->len = send(sock, ps->buf, ps->len, 0);
	
	/* 通知发送完成 */
	evsk->send_done(ps->token, ps->buf, ps->len);
	
	/* 释放EV_SEND */
	delete ps;
    
    /* 检查FIFO是否还存在待发送内容 */
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
		
	/* 把当前缓冲区挂入写FIFO */
	EV_SEND *send = new EV_SEND(token, buf, size);
	empty = wq.empty();
	wq.push(send);
	
	/* 触发写事件 */
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

