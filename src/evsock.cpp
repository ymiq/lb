#include <cstring>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/eventfd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <event.h>
#include <utils/log.h>
#include <evobj.h>
#include <evsock.h>


evsock::evsock(int fd, struct event_base* base): sockfd(fd), evbase(base) {
	/* 初始化发送优先级队列 */
	int qsize[] = {1024, 2048, 4096, 8192};
	if (!wq.init(4, qsize)) {
		throw "can't create priority queue for evsock";
	}
	
	/* 设置句柄为非阻塞 */
	evutil_make_socket_nonblocking(fd);
		
	/* 创建Eventfd，用于线程间数据包传递 */
	efd = eventfd(0, EFD_NONBLOCK);
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
	
	/* 初始化Write Event */
	event_set(&write_ev, fd, EV_WRITE, do_write, this);
	if (event_base_set(evbase, &write_ev) < 0) {
		throw "event_base_set error";
	}
	
	/* 成员变量初始化 */
  	frag_flag = false;
  	pevobj = NULL;
}


evsock::~evsock() {
	/* 删除线程触发事件 */
	event_del(&thw_ev);
	
	/* 删除读事件 */
	event_del(&read_ev);
	
	/* 删除写事件 */
	if (!wq.empty()) {
		event_del(&write_ev);
	}

	/* 释放对象 */
	job_queue<ev_job*> *q = this->ev_queue();
	ev_job *job = q->pop();
	while (job) {
		/* 无法发送通知，直接删除？！ */
		if (job->qao) {
			delete job->qao;
		}
		if (job->buf) {
			delete[] job->buf;
		}
		delete job;
		job = q->pop();
	}

	/* 关闭句柄 */
	close(sockfd);
	close(efd);
}


void evsock::ev_close(void) {
	if (pevobj) {
		pevobj->ev_close();
	} else {
		delete this;
	}
}


void evsock::evobj_bind(evobj *pobj) {
	pevobj = pobj;
}


void evsock::frag_prepare(void *buf, size_t len, size_t off, bool bsec) {
	frag_flag = false;
	frag_sec = bsec;
	frag_buf = (char*)buf;
	frag_len = len;
	frag_off = off;
}


/* 
 * 接收分片报文
 */
int evsock::ev_recv_frag(size_t &len, bool &partition) {
	
	/* 读取数据 */
	size_t recv_len = frag_len - frag_off;
	char *recv_buf = frag_buf + frag_off;
	int ret_len = recv(sockfd, recv_buf, recv_len, MSG_DONTWAIT);
	if (ret_len <= 0) {
		if ((errno == EAGAIN) || (errno == EINTR)) {
			LOGE("recv warning: %s", strerror(errno));
			partition = true;
			frag_flag = true;
			len = 0;
			return 0;
		}
		partition = false;
		frag_flag = false;
		len = ret_len;
		LOGE("recv error: %s", strerror(errno));
	} else if (ret_len != (int)recv_len) {
		/* 非阻塞模式下，数据接收未完成 */
		partition = true;
		frag_flag = true;
		frag_off += ret_len;
		len = frag_off;
	} else {
		partition = frag_sec;
		frag_flag = false;
		len = frag_len;
	}
	return ret_len;
}


/* 
 * 接收分片/分段报文时，其不可能乱序，乱序则表示错误
 * 返回值：len <= 0: 表示发送端关闭，或者读出错
 *         len > 0: 如果partition为true，表示数据未读完，继续读，此时返回NULL
 *                  如果partition为false，返回值为NULL，表示接收数据有问题，被丢弃
 *                  如果partition为false，返回值不为NULL，表示接收到len长度数据
 */
void *evsock::ev_recv(size_t &len, bool &partition) {
	unsigned int len_off;
	unsigned int frag;
	char *buffer = NULL;
	size_t recv_len;
	serial_data *phd = &frag_header;
	int ret_len;
	
	/* 检查是否接收未完成的分片数据 */
	if (frag_flag) {
		ret_len = ev_recv_frag(len, partition);
		if (partition) {
			/* 等待继续调用ev_recv */
			return NULL;
		} else if (ret_len <= 0) {
			/* 数据读错误，或者分片数据仍旧未读完 */
			if (frag_buf != (char*)&frag_header) {
				delete[] sec_buf;
			}
			return NULL;
		}
		
		/* 无继续分片，并且当前读写不为头数据 */
		if (frag_buf != (char*)&frag_header) {
			return sec_buf;
		} 
	} else {
		/* 接收准备 */
		len = sizeof(serial_data);
		frag_prepare(phd, len, 0, false);

		/* 接收头信息 */
		ret_len = ev_recv_frag(len, partition);
		if ((ret_len <= 0) || partition) {
			/* 数据读错误，或者分片数据仍旧未读完 */
			return NULL;
		}
	}
		
	/* 局部变量初始化 */
	ret_len = sizeof(serial_data);
	len_off = phd->length & 0x3fffffffu;
	frag = phd->length >> 30;
	recv_len = phd->datalen;
	
	/* 检查头部信息是否合法 */
	unsigned long token = phd->token;
	if (frag && (!token || (token == -1ul))) {
		LOGE("recv packet with bad header");
		len = ret_len;
		return NULL;
	}
	
	/* 无分片情况 */
	if (frag == 0) {
		/* 超过最大分段长度的数据包即为不合法 */
		if ((len_off != recv_len + sizeof(serial_data))
			|| (len_off > CFG_SECTION_SIZE)) {
			LOGE("recv packet with inavalid length");
			len = ret_len;
			return NULL;
		}

		/* 申请缓冲区 */
		buffer = new char[len_off];
		sec_len = len_off;
		sec_buf = buffer;
		frag_prepare(buffer, len_off, sizeof(serial_data), false);
		
		/* 复制头部信息 */
		memcpy(buffer, phd, sizeof(serial_data));	
	} else if (frag == 1) { /* 第一个分片包 */
		/* 分片数据包总长度超过8M认为不合法 */
		if ((len_off <= recv_len + sizeof(serial_data)) 
			|| (len_off >= 0x800000)) {
			LOGE("recv packet with bad length");
			len = ret_len;
			return NULL;
		}
		
		/* 申请分片数据保存结构体 */
		buffer = new char[len_off];
		sec_token = token;
		sec_len = len_off;
		sec_off = recv_len + sizeof(serial_data); 
		sec_buf = buffer;
		frag_prepare(buffer, len_off, sizeof(serial_data), true);
		
		/* 复制头信息，转换为非分片包 */
		phd->length = len_off;
		phd->datalen = len_off - sizeof(serial_data);
		memcpy(buffer, phd, sizeof(serial_data));
	} else if (frag == 2) { /* 中间分片包 */

		buffer = sec_buf + sec_off;
		sec_off += recv_len;
		frag_prepare(buffer, recv_len, 0, true);		
	} else { /* 结束分片包 */

		buffer = sec_buf + sec_off;
		sec_off += recv_len;
		frag_prepare(buffer, recv_len, 0, false);
	}	

	/* 准备接收缓冲区 */
	ret_len = ev_recv_frag(len, partition);
	if (partition) {
		return NULL;
	} else if (ret_len <= 0) {
   		delete[] sec_buf;
 		return NULL;
	} 
	
	return sec_buf;
}


void evsock::recv_done(void *buf) {
	delete[] (char *)buf;
}


void evsock::do_write(int sock, short event, void* arg) {
	bool del_job = false;
	evsock *evsk = (evsock *)arg;
	job_queue<ev_job*> *q = evsk->ev_queue();
	
	/* 从写缓冲队列读取一个job */
	ev_job *job = q->pop();
	if (job == NULL) {
		/* 可能会出错 */
		LOGW("something error while pop from queue");
		return;
	}
	
	/* 发送数据 */
	if (job->section) {
		bool send_status = false;
		
		/* 发送分片数据 */
		char *buf = job->buf + job->offset - sizeof(serial_data);
		serial_data *pheader = (serial_data*) buf;
		size_t datalen = job->len - job->offset;
		size_t max_len = CFG_SECTION_SIZE - sizeof(serial_data);
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
	 			delete[] job->buf;
 			} else {
				evsk->send_done((void*)job->buf, job->len, send_status);
 			}
 			del_job = true;
  		}

	} else if (job->qao) {
		size_t datalen;
		
		/* 序列化对象并发送 */
		char *buf = job->qao->serialization(datalen);
		if (buf) {
			bool send_done = true;
			bool send_status = false;
			
			if (datalen > CFG_SECTION_SIZE) {
				/* 发送第一个分片数据 */
				serial_data *pheader = (serial_data*)buf;
				job->section = true;
				job->buf = buf;
				job->len = datalen;
				job->offset = CFG_SECTION_SIZE;
				job->header = *pheader;
				pheader->length |= (1u << 30);
				pheader->datalen = CFG_SECTION_SIZE - sizeof(serial_data);
				datalen = CFG_SECTION_SIZE;
				send_done = false;
			}
			
			/* 发送数据 */
			int len = send(sock, buf, datalen, 0);
			if (len == (int)datalen) {
				send_status = true;
			} else if (len > 0) {
				LOGW("Send buff full");
			}
			
			/* 发送完成或者发送失败时，通知发送完成 */
			if (send_done || !send_status) {
				evsk->send_done(job->qao, send_status);
				delete[] buf;
				del_job = true;
			}
		} else {
			evsk->send_done(job->qao, false);
			del_job = true;
		}
		
	} else {
		bool send_done = true;
		bool send_status = false;
		size_t datalen = job->len;
		
		if (job->len > CFG_SECTION_SIZE) {
			/* 发送第一个分片数据 */
			serial_data *pheader = (serial_data*)job->buf;
			job->section = true;
			job->offset = CFG_SECTION_SIZE;
			job->header = *pheader;
			pheader->length |= (1u << 30);
			pheader->datalen = CFG_SECTION_SIZE - sizeof(serial_data);
			datalen = CFG_SECTION_SIZE;
			send_done = false;
		}
		
		/* 发送缓冲区中内容 */
		int len = send(sock, job->buf, datalen, 0);
		if (len == (int)job->len) {
			send_status = true;
		} else if (len > 0) {
			LOGW("Send buff full");
		}
		datalen = len;
		
		/* 发送完成或者发送失败时，通知发送完成 */
		if (send_done || !send_status) {
			evsk->send_done((void*)job->buf, datalen, send_status);
			del_job = true;
		}
	}
	
	if (del_job) {
		/* 删除队列中的job */
		delete job;
	} else {
		/* 把job放入最高优先级队列 */
		q->push(job, 0);
	}
		
	/* 检查FIFO是否还存在待发送内容 */
	if (!q->empty() && !event_pending(&evsk->write_ev, EV_WRITE, NULL)) {
		if (event_add(&evsk->write_ev, NULL) < 0) {
			LOGE("event_add error\n");
		}
	}
}


bool evsock::ev_send(const void *buf, size_t size, int qos) {
	if (!buf || !size) {
		return false;
	}
		
	/* 把当前缓冲区挂入写FIFO */
	ev_job *job = new ev_job((char*)buf, size, NULL);
	bool ret = wq.push(job, qos);
	if (ret) {
		/* 触发写事件 */
		if (!wq.empty() && !event_pending(&write_ev, EV_WRITE, NULL)) {
			if (event_add(&write_ev, NULL) < 0) {
				LOGE("event_add error\n");
				return false;
			}
		}
	}
	return ret;
}



bool evsock::ev_send(const void *buf, size_t size) {
	return ev_send(buf, size, 3);
}


bool evsock::ev_send(qao_base *qao) {
	if (!qao) {
		return false;
	}
		
	/* 把当前缓冲区挂入写FIFO */
	ev_job *job = new ev_job(NULL, 0, qao);
	bool ret = wq.push(job, qao->get_qos());
	if (ret) {
		/* 触发写事件 */
		if (!wq.empty() && !event_pending(&write_ev, EV_WRITE, NULL)) {
			if (event_add(&write_ev, NULL) < 0) {
				LOGE("event_add error\n");
				return false;
			}
		}
	}
	return ret;
}


bool evsock::ev_send_inter_thread(const void *buf, size_t size, int qos) {
	if (!buf || !size) {
		return false;
	}
	
	/* 把当前缓冲区挂入写FIFO */
	ev_job *job = new ev_job((char*)buf, size, NULL);
	bool ret = wq.push(job, qos); 
	if (ret) {
		/* 触发写事件 */
		unsigned long cnt = 1;
		write(efd, &cnt, sizeof(cnt));
	}
	return ret;
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
	bool ret = wq.push(job, 3); 
	if (ret) {
		/* 触发写事件 */
		unsigned long cnt = 1;
		write(efd, &cnt, sizeof(cnt));
	}
	return ret;
}


void evsock::do_eventfd(int efd, short event, void* arg) {
	evsock *evsk = (evsock *)arg;
	job_queue<ev_job*> *q = evsk->ev_queue();
	
	/* 清除事件 */
	unsigned long cnt;
	read(evsk->efd, &cnt, sizeof(cnt));
	
	/* 检查Queue不为空，当前没有EV_WRITE事件 */
	if (!q->empty() && !event_pending(&evsk->write_ev, EV_WRITE, NULL)) {
		if (event_add(&evsk->write_ev, NULL) < 0) {
			LOGE("event_add error\n");
		}
	}
}

