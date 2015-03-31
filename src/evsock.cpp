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
	int qsize[] = {1024, 8192, 8192, 16384};
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
  	wfrag_flag = false;
  	sock_error = false;
  	pevobj = NULL;
}


evsock::~evsock() {
	/* ɾ���̴߳����¼� */
	event_del(&thw_ev);
	
	/* ɾ�����¼� */
	event_del(&read_ev);
	
	/* ɾ��д�¼� */
	event_del(&write_ev);

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


void evsock::frag_prepare(void *buf, size_t len, size_t off, int bsec) {
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
			partition = true;
			frag_flag = true;
			len = 0;
			return 0;
		}
		partition = false;
		frag_flag = false;
		len = ret_len;
		if (ret_len < 0) {
			sock_error = true;
			LOGD("recv error: %s", strerror(errno));
		}
	} else if (ret_len != (int)recv_len) {
		/* ������ģʽ�£����ݽ���δ��� */
		partition = true;
		frag_flag = true;
		frag_off += ret_len;
		len = frag_off;
	} else {
		frag_flag = false;
		if (frag_sec == 0) {
			partition = false;
			len = frag_len;
		} else if ((frag_sec == 1) || (frag_sec == 2)) {
			partition = true;
			len = sec_off;
		} else if (frag_sec == 3) {
			partition = false;
			len = sec_off;
		}
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
		frag_prepare(phd, len, 0, 0);

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
	
	/* �޷�Ƭ��� */
	if (frag == 0) {
		/* �������ֶγ��ȵ����ݰ���Ϊ���Ϸ� */
		if ((len_off != recv_len + sizeof(serial_data))
			|| (len_off > CFG_SECTION_SIZE)) {
			LOGE("recv packet with inavalid length: %d, %d", len_off, recv_len);
			len = ret_len;
			return NULL;
		}

		/* ���뻺���� */
		buffer = new char[len_off];
		sec_len = len_off;
		sec_buf = buffer;
		frag_prepare(buffer, len_off, sizeof(serial_data), 0);
		
		/* ����ͷ����Ϣ */
		memcpy(buffer, phd, sizeof(serial_data));	
	} else if (frag == 1) { /* ��һ����Ƭ�� */
		/* ��Ƭ���ݰ��ܳ��ȳ���8M��Ϊ���Ϸ� */
		if ((len_off <= recv_len + sizeof(serial_data)) 
			|| (len_off >= 0x800000)) {
			LOGE("recv packet with bad length: %d, %d", len_off, recv_len);
			len = ret_len;
			return NULL;
		}
		
		/* �����Ƭ���ݱ���ṹ�� */
		buffer = new char[len_off];
		sec_token = phd->token;
		sec_len = len_off;
		sec_off = recv_len + sizeof(serial_data); 
		sec_buf = buffer;
		frag_prepare(buffer, sec_off, sizeof(serial_data), 1);
		
		/* ����ͷ��Ϣ��ת��Ϊ�Ƿ�Ƭ�� */
		phd->length = len_off;
		phd->datalen = len_off - sizeof(serial_data);
		memcpy(buffer, phd, sizeof(serial_data));
	} else if (frag == 2) { /* �м��Ƭ�� */

		buffer = sec_buf + sec_off;
		sec_off += recv_len;
		frag_prepare(buffer, recv_len, 0, 2);		
	} else { /* ������Ƭ�� */

		buffer = sec_buf + sec_off;
		sec_off += recv_len;
		frag_prepare(buffer, recv_len, 0, 3);
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


void evsock::wfrag_prepare(evsock *evsk, ev_job* job, void *buf, size_t len, int type) {
	evsk->wfrag_flag = true;
	evsk->wfrag_buf = (char*)buf;
	evsk->wfrag_len = len;
	evsk->wfrag_off = 0;
	evsk->wfrag_job = job;
	evsk->wfrag_type = type;
}


/* 
 * ����ָ�����������ݣ�����EAGAIN��EINTR����
 */
void evsock::do_write_buffer(evsock *evsk) {
	char *wbuf = evsk->wfrag_buf + evsk->wfrag_off;
	size_t wlen = evsk->wfrag_len - evsk->wfrag_off;
	ev_job *job = evsk->wfrag_job;
		
	/* �������� */
	int len = send(evsk->sockfd, wbuf, wlen, MSG_DONTWAIT);
	if (len <= 0) {
		/* ������ */
		if ((errno != EAGAIN) && (errno != EINTR)) {
			evsk->wfrag_flag = false;
			if (evsk->wfrag_type != 1) {
				/* ֪ͨ������� */
	 			if (job->qao) {
					evsk->send_done(job->qao, false);
		 			delete[] job->buf;
	 			} else {
					evsk->send_done((void*)job->buf, job->len, false);
	 			}
	 		}
			LOGD("send error: %s", strerror(errno));
			evsk->sock_error = true;
 			delete job;
		}
		len = 0;
	} 
	
	/* ֻ���Ͳ������ݣ��ȴ��������� */
	if (len != (int)wlen) {
		evsk->wfrag_off += len;
		
		/* ����¼����ȴ��������� */
		if (!event_pending(&evsk->write_ev, EV_WRITE, NULL)) {
			if (event_add(&evsk->write_ev, NULL) < 0) {
				LOGE("event_add error, check it\n");
			}
		}
		return;
	}

	/* ���ݷ������ */
	evsk->wfrag_flag = false;
	job_queue<ev_job*> *q = evsk->ev_queue();
	if (evsk->wfrag_type == 0) {
		/* ֪ͨ������� */
		if (job->qao) {
			evsk->send_done(job->qao, true);
			delete[] job->buf;
		} else {
			evsk->send_done((void*)job->buf, job->len, true);
		}
		delete job;
	} else if (evsk->wfrag_type == 1) {
		
		/* SECTION����ͷ��Ϣ������ϣ����ͺ������� */
		serial_data *pheader = &job->header;
		size_t datalen = pheader->datalen;
		char *buf = job->buf + job->offset;
		job->offset += datalen;
		
		int type = 0;
		if (job->offset != job->len) {
			type = 2;
		}
		wfrag_prepare(evsk, job, buf, datalen, type);
		
		do_write_buffer(evsk);
		return;
		
	} else if (evsk->wfrag_type == 2) {
		q->push(job, 0);
	}
	
	/* ���FIFO�Ƿ񻹴��ڴ��������� */
	if (!q->empty() && !event_pending(&evsk->write_ev, EV_WRITE, NULL)) {
		if (event_add(&evsk->write_ev, NULL) < 0) {
			LOGE("event_add error\n");
		}
	}
}


void evsock::do_write(int sock, short event, void* arg) {
	evsock *evsk = (evsock *)arg;
	
	/* ����Ƿ����д�루����������*/
	if (evsk->wfrag_flag) {
		do_write_buffer(evsk);
		return;
	}
	
	/* ��д������ж�ȡһ��job */
	job_queue<ev_job*> *q = evsk->ev_queue();
	ev_job *job = q->pop();
	if (job == NULL) {
		LOGE("impossible error when pop from queue, check it");
		return;
	}
	
	/* �������� */
	if (job->section) {
		/* ���ͷ�Ƭ���� */
		size_t datalen = job->len - job->offset;
		size_t max_len = CFG_SECTION_SIZE - sizeof(serial_data);
		serial_data *pheader = &job->header;
   		if (datalen > max_len) {
   			datalen = max_len;
			pheader->length = job->offset | (2u << 30);
			pheader->datalen = datalen;
		} else {
			pheader->length = job->offset | (3u << 30);
		}
		pheader->datalen = datalen;
		
		/* ��������  */
		wfrag_prepare(evsk, job, pheader, sizeof(serial_data), 1);
		do_write_buffer(evsk);

	} else if (job->qao) {
		size_t datalen;
		
		/* ���л����󲢷��� */
		char *buf = job->qao->serialization(datalen);
		if (buf) {
			int type = 0;
			job->buf = buf;
			if (datalen > CFG_SECTION_SIZE) {
				/* ���͵�һ����Ƭ���� */
				serial_data *pheader = (serial_data*)buf;
				job->section = true;
				job->len = datalen;
				job->offset = CFG_SECTION_SIZE;
				job->header = *pheader;
				pheader->length |= (1u << 30);
				pheader->datalen = CFG_SECTION_SIZE - sizeof(serial_data);
				datalen = CFG_SECTION_SIZE;
				type = 2;
			}
				
			/* �������� */
			wfrag_prepare(evsk, job, buf, datalen, type);
			do_write_buffer(evsk);
			
		} else {
			evsk->send_done(job->qao, false);
			delete job;
		}
		
	} else {
		int type = 0;
		size_t datalen = job->len;
		
		if (datalen > CFG_SECTION_SIZE) {
			/* ���͵�һ����Ƭ���� */
			serial_data *pheader = (serial_data*)job->buf;
			job->section = true;
			job->offset = CFG_SECTION_SIZE;
			job->header = *pheader;
			pheader->length |= (1u << 30);
			pheader->datalen = CFG_SECTION_SIZE - sizeof(serial_data);
			datalen = CFG_SECTION_SIZE;
			type = 2;
		}
		
		/* ��������  */
		wfrag_prepare(evsk, job, job->buf, datalen, type);
		do_write_buffer(evsk);
	}
}


bool evsock::ev_send(const void *buf, size_t size, int qos) {
	if (!buf || !size || sock_error) {
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
	} else {
		delete job;
	}
	return ret;
}



bool evsock::ev_send(const void *buf, size_t size) {
	return ev_send(buf, size, 3);
}


bool evsock::ev_send(qao_base *qao) {
	if (!qao || sock_error) {
		return false;
	}
		
	/* �ѵ�ǰ����������дFIFO */
	ev_job *job = new ev_job(NULL, 0, qao);
	int qos = qao->get_qos();
	if (qos == 0) {
		LOGW("QAO with PRI 0 is forbidden, check it");
		qos = 3;
	}
	bool ret = wq.push(job, qos);
	if (ret) {
		/* ����д�¼� */
		if (!wq.empty() && !event_pending(&write_ev, EV_WRITE, NULL)) {
			if (event_add(&write_ev, NULL) < 0) {
				LOGE("event_add error\n");
				return false;
			}
		}
	} else {
		delete job;
	}
	return ret;
}


bool evsock::ev_send_inter_thread(const void *buf, size_t size, int qos) {
	if (!buf || !size || sock_error) {
		return false;
	}
	
	/* �ѵ�ǰ����������дFIFO */
	ev_job *job = new ev_job((char*)buf, size, NULL);
	bool ret = wq.push(job, qos); 
	if (ret) {
		/* ����д�¼� */
		unsigned long cnt = 1;
		write(efd, &cnt, sizeof(cnt));
	} else {
		delete job;
	}
	return ret;
}



bool evsock::ev_send_inter_thread(const void *buf, size_t size) {
	return ev_send_inter_thread(buf, size, 3);
}


bool evsock::ev_send_inter_thread(qao_base *qao) {
	if (!qao || sock_error) {
		return false;
	}
	
	/* �ѵ�ǰ����������дFIFO */
	ev_job *job = new ev_job(NULL, 0, qao);
	bool ret = wq.push(job, 3); 
	if (ret) {
		/* ����д�¼� */
		unsigned long cnt = 1;
		write(efd, &cnt, sizeof(cnt));
	} else {
		delete job;
	}
	return ret;
}


void evsock::do_eventfd(int efd, short event, void* arg) {
	evsock *evsk = (evsock *)arg;
	
	/* ����¼� */
	unsigned long cnt;
	if (read(evsk->efd, &cnt, sizeof(cnt)) <= 0) {
		if ((errno == EAGAIN) || (errno == EINTR)) {
			return;
		}
		LOGE("read eventfd error");
		return;
	}
	
	/* ���Queue��Ϊ�գ���ǰû��EV_WRITE�¼� */
	job_queue<ev_job*> *q = evsk->ev_queue();
	if (!q->empty() && !event_pending(&evsk->write_ev, EV_WRITE, NULL)) {
		if (event_add(&evsk->write_ev, NULL) < 0) {
			LOGE("event_add error\n");
		}
	}
}

