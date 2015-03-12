#include <cstring>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/eventfd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <event.h>
#include <log.h>
#include <evsock.h>


evsock::evsock(int fd, struct event_base* base): sockfd(fd), evbase(base) {
	int qsize[] = {1024, 2048, 4096, 8192};
	if (!wq.init(4, qsize)) {
		throw "can't create priority queue for evsock";
	}
	
	/* 创建Eventfd，用于线程间数据包传递 */
#if 0	
	efd = eventfd(0, 0);
	if (efd < 0) {
		throw "can't creat evnentfd for evsock";
	}
	event_set(&thw_ev, efd, EV_READ|EV_PERSIST, do_eventfd, this);
	if (event_base_set(base, &thw_ev) < 0) {
		throw "event_base_set error";
	}
	if (event_add(&thw_ev, NULL) < 0) {
		throw "event_add error";
	}
#endif
}


evsock::~evsock() {
	/* 删除读事件 */
    event_del(&read_ev);
    
    /* 删除写事件 */
	if (!wq.empty()) {
	    event_del(&write_ev);
	}
	
	/* 释放对象 */
	pri_queue<ev_job*> *q = this->ev_queue();
    ev_job *job = q->pop();
	while (job) {
		/* 无法发送通知，直接删除？！ */
		if (job->qao) {
			delete job->qao;
			delete job->buf;
		} else {
			delete job->buf;
  		}
		delete job;
	    job = q->pop();
	}
}


/* 接收分片报文时，认为其不可能乱序，乱序则表示错误
 * 该Socket为内部主机间通信Socket，乱序是不可能发生的
 */
void *evsock::ev_recv(size_t &len, bool &fragment) {
	unsigned int len_off;
	unsigned int frag;
	frag_pack *pfrag = NULL;
	char *buffer = NULL;
	char *precv = NULL;
	size_t recv_len;
	
	fragment = false;
	
	/* 接收头信息 */
	serial_data header;
    int rd_size = recv(sockfd, &header, sizeof(serial_data), MSG_DONTWAIT);
    if (rd_size != sizeof(serial_data)) {
    	if (rd_size > 0) {
   			rd_size = -1;
   		}
		len = rd_size;
		return NULL;
    }
    rd_size = -1;
    len_off = header.length & 0x3fffffffu;
    frag = header.length >> 30;
	recv_len = header.datalen;
    
    /* 检查token是否合法 */
	unsigned long token = header.token;
	if (frag && (!token || (token == -1ul))) {
		LOGE("recv packet with bad token");
		goto error_quit;
	}
 	
	/* 长度合法性检查 */
	switch(frag) {
		/* 无分片情况 */
	    case 0: 
	    	/* 超过64K数据包为不合法 */
		    if ((len_off != recv_len + sizeof(serial_data))
		    	|| (len_off >= 0x10000)) {
				LOGE("recv packet with bad length");
				goto error_quit;
			}
		    buffer = new char[len_off];
			precv = buffer + sizeof(serial_data);
			
			/* 复制头信息 */
			memcpy(buffer, &header, sizeof(serial_data));
			len = len_off;
			break;
		
		/* 第一个分片包 */
		case 1:
			fragment = true;
			
	    	/* 分片数据包总长度超过8M认为不合法 */
			if ((len_off <= recv_len + sizeof(serial_data)) 
				|| (len_off >= 0x800000)) {
				LOGE("recv packet with bad length");
				goto error_quit;
			}
			
			/* 申请分片数据保存结构体 */
			pfrag = frags.get(token);
			if (pfrag == NULL) {
				LOGE("error_quit fragment");
				goto error_quit;
			}
			pfrag->token = token;
			pfrag->length = len_off;
			pfrag->rcvlen = recv_len + sizeof(serial_data); 
			pfrag->time = 0;
		    buffer = new char[len_off];
		    pfrag->buffer = buffer;
			precv = buffer + sizeof(serial_data);
		    
			/* 复制头信息，转换为非分片包 */
		    header.length = len_off;
		    header.datalen = len_off - sizeof(serial_data);
			memcpy(buffer, &header, sizeof(serial_data));
			len = recv_len;
			break;
    	
		/* 中间分片包 */
    	case 2:
    	case 3:
			fragment = true;
			
			/* 获取分片数据保存结构体 */
			pfrag = frags.find(token);
			if (pfrag == NULL) {
				LOGE("recv fragment with bad token");
				goto error_quit;
			}
			pfrag->rcvlen += recv_len;
			if (frag == 3) {
 				if (pfrag->rcvlen != pfrag->length) {
					LOGE("recv fragment with bad length");
					delete pfrag->buffer;
					frags.remove(token);
					goto error_quit;
				} else {
					fragment = false;
					frags.remove(token);
		 			buffer = pfrag->buffer;
		 			precv = buffer + len_off;
		 			len = pfrag->length;
				}
 			} else {
	 			buffer = pfrag->buffer;
	 			precv = buffer + len_off;
	 			len = recv_len;
	 		}
	   		break;
	}    	
    
    /* 准备接收缓冲区 */
    rd_size = recv(sockfd, precv, recv_len, MSG_DONTWAIT);
    if (rd_size != (int)recv_len) {
    	LOGE("recv length umatch, want %d but %d", recv_len, rd_size);
   		delete buffer;
    	if (fragment) {
			frags.remove(token);
   		}
    	if (rd_size > 0) {
   			rd_size = -1;
   		}
    	goto error_quit;
    }
    
    /* 分片处理 */
    if (fragment) {
 		return NULL;
    }
   	return buffer;
    
error_quit:
	len = rd_size;
	fragment = false;
	return NULL;
}


void evsock::recv_done(void *buf) {
	delete (char *)buf;
}


void evsock::do_write(int sock, short event, void* arg) {
	bool pop_job = false;
	evsock *evsk = (evsock *)arg;
	pri_queue<ev_job*> *q = evsk->ev_queue();
	
    /* 从写缓冲队列读取一个job */
    ev_job *job = q->front();
    if (job == NULL) {
    	/* 过多事件触发会导致Job为空 */
    	return;
    }
    
    /* 发送数据 */
    if (job->fragment) {
    	bool send_status = false;
    	
    	/* 发送分片数据 */
    	char *buf = job->buf + job->offset - sizeof(serial_data);
    	serial_data *pheader = (serial_data*) buf;
    	size_t datalen = job->len - job->offset;
    	size_t max_len = CFG_FRAGMENT_SIZE - sizeof(serial_data);
   		*pheader = job->header;
   		if (datalen > max_len) {
    		pheader->length = job->offset | (2u << 30);
    		pheader->datalen = max_len;
    		datalen = max_len;
    	} else {
    		pheader->length = job->offset | (3u << 30);;
    		pheader->datalen = datalen;
    	}
    	job->offset += datalen;
    	datalen += sizeof(serial_data);
    	
    	/* 发送数据头 */
		int len = send(sock, buf, datalen, 0);
		if (len == (int)datalen) {
			send_status = true;
		}
				    		
		/* 通知发送完成 */
   		if ((job->offset == job->len) || !send_status) {
 			if (job->qao) {
				evsk->send_done(job->qao, send_status);
	 			delete job->buf;
 			} else {
				evsk->send_done((void*)job->buf, job->len, send_status);
 			}
 			pop_job = true;
  		}

    } else if (job->qao) {
		size_t datalen;
		
		/* 序列化对象并发送 */
		char *buf = job->qao->serialization(datalen);
		if (buf) {
			bool send_done = true;
			bool send_status = false;
			
			if (datalen > CFG_FRAGMENT_SIZE) {
				/* 发送第一个分片数据 */
				serial_data *pheader = (serial_data*)buf;
				job->fragment = true;
				job->buf = buf;
				job->len = datalen;
				job->offset = CFG_FRAGMENT_SIZE;
				job->header = *pheader;
				pheader->length |= (1u << 30);
				pheader->datalen = CFG_FRAGMENT_SIZE - sizeof(serial_data);
				datalen = CFG_FRAGMENT_SIZE;
				send_done = false;
			}
			
			/* 发送数据 */
			int len = send(sock, buf, datalen, 0);
			if (len == (int)datalen) {
				send_status = true;
			}
			
			/* 发送完成或者发送失败时，通知发送完成 */
			if (send_done || !send_status) {
				evsk->send_done(job->qao, send_status);
				delete buf;
				pop_job = true;
			}
		} else {
			evsk->send_done(job->qao, false);
			pop_job = true;
		}
		
    } else {
    	bool send_done = true;
		bool send_status = false;
		size_t datalen = job->len;
		
		if (job->len > CFG_FRAGMENT_SIZE) {
			/* 发送第一个分片数据 */
			serial_data *pheader = (serial_data*)job->buf;
			job->fragment = true;
			job->offset = CFG_FRAGMENT_SIZE;
			job->header = *pheader;
			pheader->length |= (1u << 30);
			pheader->datalen = CFG_FRAGMENT_SIZE - sizeof(serial_data);
			datalen = CFG_FRAGMENT_SIZE;
			send_done = false;
		}
		
		/* 发送缓冲区中内容 */
		int len = send(sock, job->buf, datalen, 0);
		if (len == (int)job->len) {
			send_status = true;
		}
		datalen = len;
		
		/* 发送完成或者发送失败时，通知发送完成 */
		if (send_done || !send_status) {
			evsk->send_done((void*)job->buf, datalen, send_status);
			pop_job = true;
		}
	}
	
	/* 删除队列中的job */
	if (pop_job) {
		q->pop(job);
		delete job;
	}
		
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


bool evsock::ev_send(const void *buf, size_t size, int qos) {
	bool empty;
	
	if (!buf || !size) {
		return false;
	}
		
	/* 把当前缓冲区挂入写FIFO */
	ev_job *job = new ev_job((char*)buf, size, NULL);
	empty = wq.empty();
	wq.push(job, qos);
	
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



bool evsock::ev_send(const void *buf, size_t size) {
    return ev_send(buf, size, 3);
}


bool evsock::ev_send(qao_base *qao) {
	bool empty;
	
	if (!qao) {
		return false;
	}
		
	/* 把当前缓冲区挂入写FIFO */
	ev_job *job = new ev_job(NULL, 0, qao);
	empty = wq.empty();
	wq.push(job, qao->get_qos());
	
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


bool evsock::ev_send_inter_thread(const void *buf, size_t size, int qos) {
	if (!buf || !size) {
		return false;
	}
		
	/* 把当前缓冲区挂入写FIFO */
	ev_job *job = new ev_job((char*)buf, size, NULL);
	wq.push(job, qos);
	
	/* 触发写事件 */
	unsigned long cnt = 1;
	write(efd, &cnt, sizeof(cnt));
    return true;
}



bool evsock::ev_send_inter_thread(const void *buf, size_t size) {
    return ev_send_inter_thread(buf, size, 3);
}


bool evsock::ev_send_inter_thread(qao_base *qao) {
	if (!qao) {
		return false;
	}
		
	/* 把当前缓冲区挂入写FIFO */
	ev_job *job = new ev_job(NULL, 0, qao);
	wq.push(job, qao->get_qos());
	
	/* 触发写事件 */
	unsigned long cnt = 1;
	write(efd, &cnt, sizeof(cnt));
    return true;
}


void evsock::do_eventfd(int efd, short event, void* arg) {
	evsock *evsk = (evsock *)arg;
	pri_queue<ev_job*> *q = evsk->ev_queue();
	
    /* 检查FIFO是否还存在待发送内容 */
    if (!q->empty()) {
	    event_set(&evsk->write_ev, evsk->sockfd, EV_WRITE, do_write, evsk);
	    if (event_base_set(evsk->evbase, &evsk->write_ev) < 0) {
	    	LOGE("event_base_set error\n");
	    	return;
	    }
	    if (event_add(&evsk->write_ev, NULL) < 0) {
	    	LOGE("event_add error\n");
	    }
    }
}


