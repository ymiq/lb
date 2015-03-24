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
	/* ��ʼ���������ȼ����� */
	int qsize[] = {1024, 2048, 4096, 8192};
	if (!wq.init(4, qsize)) {
		throw "can't create priority queue for evsock";
	}
	
	/* ���þ��Ϊ������ */
	evutil_make_socket_nonblocking(fd);
		
	/* ����Eventfd�������̼߳����ݰ����� */
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
	
	/* ��ʼ��Write Event */
	event_set(&write_ev, fd, EV_WRITE, do_write, this);
	if (event_base_set(evbase, &write_ev) < 0) {
		throw "event_base_set error";
	}
	
	/* ��Ա������ʼ�� */
  	frag_flag = false;
  	pevobj = NULL;
}


evsock::~evsock() {
	/* ɾ���̴߳����¼� */
	event_del(&thw_ev);
	
	/* ɾ�����¼� */
	event_del(&read_ev);
	
	/* ɾ��д�¼� */
	if (!wq.empty()) {
		event_del(&write_ev);
	}

	/* �ͷŶ��� */
	job_queue<ev_job*> *q = this->ev_queue();
	ev_job *job = q->pop();
	while (job) {
		/* �޷�����֪ͨ��ֱ��ɾ������ */
		if (job->qao) {
			delete job->qao;
		}
		if (job->buf) {
			delete[] job->buf;
		}
		delete job;
		job = q->pop();
	}

	/* �رվ�� */
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
 * ���շ�Ƭ����
 */
int evsock::ev_recv_frag(size_t &len, bool &partition) {
	
	/* ��ȡ���� */
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
		/* ������ģʽ�£����ݽ���δ��� */
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
 * ���շ�Ƭ/�ֶα���ʱ���䲻���������������ʾ����
 * ����ֵ��len <= 0: ��ʾ���Ͷ˹رգ����߶�����
 *         len > 0: ���partitionΪtrue����ʾ����δ���꣬����������ʱ����NULL
 *                  ���partitionΪfalse������ֵΪNULL����ʾ�������������⣬������
 *                  ���partitionΪfalse������ֵ��ΪNULL����ʾ���յ�len��������
 */
void *evsock::ev_recv(size_t &len, bool &partition) {
	unsigned int len_off;
	unsigned int frag;
	char *buffer = NULL;
	size_t recv_len;
	serial_data *phd = &frag_header;
	int ret_len;
	
	/* ����Ƿ����δ��ɵķ�Ƭ���� */
	if (frag_flag) {
		ret_len = ev_recv_frag(len, partition);
		if (partition) {
			/* �ȴ���������ev_recv */
			return NULL;
		} else if (ret_len <= 0) {
			/* ���ݶ����󣬻��߷�Ƭ�����Ծ�δ���� */
			if (frag_buf != (char*)&frag_header) {
				delete[] sec_buf;
			}
			return NULL;
		}
		
		/* �޼�����Ƭ�����ҵ�ǰ��д��Ϊͷ���� */
		if (frag_buf != (char*)&frag_header) {
			return sec_buf;
		} 
	} else {
		/* ����׼�� */
		len = sizeof(serial_data);
		frag_prepare(phd, len, 0, false);

		/* ����ͷ��Ϣ */
		ret_len = ev_recv_frag(len, partition);
		if ((ret_len <= 0) || partition) {
			/* ���ݶ����󣬻��߷�Ƭ�����Ծ�δ���� */
			return NULL;
		}
	}
		
	/* �ֲ�������ʼ�� */
	ret_len = sizeof(serial_data);
	len_off = phd->length & 0x3fffffffu;
	frag = phd->length >> 30;
	recv_len = phd->datalen;
	
	/* ���ͷ����Ϣ�Ƿ�Ϸ� */
	unsigned long token = phd->token;
	if (frag && (!token || (token == -1ul))) {
		LOGE("recv packet with bad header");
		len = ret_len;
		return NULL;
	}
	
	/* �޷�Ƭ��� */
	if (frag == 0) {
		/* �������ֶγ��ȵ����ݰ���Ϊ���Ϸ� */
		if ((len_off != recv_len + sizeof(serial_data))
			|| (len_off > CFG_SECTION_SIZE)) {
			LOGE("recv packet with inavalid length");
			len = ret_len;
			return NULL;
		}

		/* ���뻺���� */
		buffer = new char[len_off];
		sec_len = len_off;
		sec_buf = buffer;
		frag_prepare(buffer, len_off, sizeof(serial_data), false);
		
		/* ����ͷ����Ϣ */
		memcpy(buffer, phd, sizeof(serial_data));	
	} else if (frag == 1) { /* ��һ����Ƭ�� */
		/* ��Ƭ���ݰ��ܳ��ȳ���8M��Ϊ���Ϸ� */
		if ((len_off <= recv_len + sizeof(serial_data)) 
			|| (len_off >= 0x800000)) {
			LOGE("recv packet with bad length");
			len = ret_len;
			return NULL;
		}
		
		/* �����Ƭ���ݱ���ṹ�� */
		buffer = new char[len_off];
		sec_token = token;
		sec_len = len_off;
		sec_off = recv_len + sizeof(serial_data); 
		sec_buf = buffer;
		frag_prepare(buffer, len_off, sizeof(serial_data), true);
		
		/* ����ͷ��Ϣ��ת��Ϊ�Ƿ�Ƭ�� */
		phd->length = len_off;
		phd->datalen = len_off - sizeof(serial_data);
		memcpy(buffer, phd, sizeof(serial_data));
	} else if (frag == 2) { /* �м��Ƭ�� */

		buffer = sec_buf + sec_off;
		sec_off += recv_len;
		frag_prepare(buffer, recv_len, 0, true);		
	} else { /* ������Ƭ�� */

		buffer = sec_buf + sec_off;
		sec_off += recv_len;
		frag_prepare(buffer, recv_len, 0, false);
	}	

	/* ׼�����ջ����� */
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
	
	/* ��д������ж�ȡһ��job */
	ev_job *job = q->pop();
	if (job == NULL) {
		/* ���ܻ���� */
		LOGW("something error while pop from queue");
		return;
	}
	
	/* �������� */
	if (job->section) {
		bool send_status = false;
		
		/* ���ͷ�Ƭ���� */
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
		
		/* ��������ͷ */
		int len = send(sock, buf, datalen, 0);
		if (len == (int)datalen) {
			send_status = true;
		}
							
		/* ֪ͨ������� */
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
		
		/* ���л����󲢷��� */
		char *buf = job->qao->serialization(datalen);
		if (buf) {
			bool send_done = true;
			bool send_status = false;
			
			if (datalen > CFG_SECTION_SIZE) {
				/* ���͵�һ����Ƭ���� */
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
			
			/* �������� */
			int len = send(sock, buf, datalen, 0);
			if (len == (int)datalen) {
				send_status = true;
			} else if (len > 0) {
				LOGW("Send buff full");
			}
			
			/* ������ɻ��߷���ʧ��ʱ��֪ͨ������� */
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
			/* ���͵�һ����Ƭ���� */
			serial_data *pheader = (serial_data*)job->buf;
			job->section = true;
			job->offset = CFG_SECTION_SIZE;
			job->header = *pheader;
			pheader->length |= (1u << 30);
			pheader->datalen = CFG_SECTION_SIZE - sizeof(serial_data);
			datalen = CFG_SECTION_SIZE;
			send_done = false;
		}
		
		/* ���ͻ����������� */
		int len = send(sock, job->buf, datalen, 0);
		if (len == (int)job->len) {
			send_status = true;
		} else if (len > 0) {
			LOGW("Send buff full");
		}
		datalen = len;
		
		/* ������ɻ��߷���ʧ��ʱ��֪ͨ������� */
		if (send_done || !send_status) {
			evsk->send_done((void*)job->buf, datalen, send_status);
			del_job = true;
		}
	}
	
	if (del_job) {
		/* ɾ�������е�job */
		delete job;
	} else {
		/* ��job����������ȼ����� */
		q->push(job, 0);
	}
		
	/* ���FIFO�Ƿ񻹴��ڴ��������� */
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
		
	/* �ѵ�ǰ����������дFIFO */
	ev_job *job = new ev_job((char*)buf, size, NULL);
	bool ret = wq.push(job, qos);
	if (ret) {
		/* ����д�¼� */
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
		
	/* �ѵ�ǰ����������дFIFO */
	ev_job *job = new ev_job(NULL, 0, qao);
	bool ret = wq.push(job, qao->get_qos());
	if (ret) {
		/* ����д�¼� */
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
	
	/* �ѵ�ǰ����������дFIFO */
	ev_job *job = new ev_job((char*)buf, size, NULL);
	bool ret = wq.push(job, qos); 
	if (ret) {
		/* ����д�¼� */
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
	
	/* �ѵ�ǰ����������дFIFO */
	ev_job *job = new ev_job(NULL, 0, qao);
	bool ret = wq.push(job, 3); 
	if (ret) {
		/* ����д�¼� */
		unsigned long cnt = 1;
		write(efd, &cnt, sizeof(cnt));
	}
	return ret;
}


void evsock::do_eventfd(int efd, short event, void* arg) {
	evsock *evsk = (evsock *)arg;
	job_queue<ev_job*> *q = evsk->ev_queue();
	
	/* ����¼� */
	unsigned long cnt;
	read(evsk->efd, &cnt, sizeof(cnt));
	
	/* ���Queue��Ϊ�գ���ǰû��EV_WRITE�¼� */
	if (!q->empty() && !event_pending(&evsk->write_ev, EV_WRITE, NULL)) {
		if (event_add(&evsk->write_ev, NULL) < 0) {
			LOGE("event_add error\n");
		}
	}
}

