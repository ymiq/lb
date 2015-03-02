
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>

#include <unistd.h>
#include <log.h>
#include <stat_man.h>
#include <lb_table.h>
#include "cmdsrv.h"

using namespace std;

cmdsrv::cmdsrv(int fd, struct event_base* base):evsock(fd, base) {
	pstat = stat_man::get_inst();
	plb = lb_table::get_inst();
}


cmdsrv::~cmdsrv() {
}


void *cmdsrv::company_stat(LB_CMD *cmd, size_t *size) {
	char *buf;
	
	unsigned int command = cmd->cmd & 0x0fffffff;
	switch(command) {
		
	/* 开启统计 */		
	case 1:
		buf = (char*)malloc(sizeof(bool));
		if (buf == NULL) {
			LOGE("No memory");
			return NULL;
		}
		*size = sizeof(bool);
		
		if (pstat->start(cmd->hash, 1) < 0) {
		    *(bool*)buf = false;
			LOGE("start statics error");
		} else {
		    *(bool*)buf = false;
		}
		break;
		
	/* 关闭统计 */	
	case 2:
		buf = (char*)malloc(sizeof(bool));
		if (buf == NULL) {
			LOGE("No memory");
			return NULL;
		}
		*size = sizeof(bool);
		
		if (pstat->start(cmd->hash, 0) < 0) {
		    *(bool*)buf = false;
			LOGE("stop statics error");
		} else {
		    *(bool*)buf = true;
		}
		break;
		
	/* 获取统计信息 */	
	case 3:
		{
			stat_info info;
			struct timeval tm;
			
			buf = (char*)malloc(sizeof(bool) + sizeof(info) + sizeof(tm));
			if (buf == NULL) {
				LOGE("No memory");
				return NULL;
			}
			*size = sizeof(bool) + sizeof(info) + sizeof(tm);
			
			if (pstat->read(cmd->hash, &info, &tm) < 0) {
			    *(bool*)buf = false;
				LOGE("set_company_stat error");
				break;
			} else {
				int len = sizeof(bool);
				
 			    *(bool*)buf = true;
	   			memcpy(buf + len, &tm, sizeof(tm));
    			len += sizeof(tm);
    			memcpy(buf + len, &info, sizeof(info));
			}
		}
		
	default:
		buf = (char*)malloc(sizeof(bool));
		if (buf == NULL) {
			LOGE("No memory");
			return NULL;
		}
		*size = sizeof(bool);
	    *(bool*)buf = false;
	}
	return buf;
}


void *cmdsrv::group_stat(LB_CMD *cmd, size_t *size) {
	return NULL;
}


void *cmdsrv::company_lb(LB_CMD *cmd, size_t *size) {
	char *buf;
	
	unsigned int command = cmd->cmd & 0x0fffffff;
	switch(command) {
		
	/* 开启LB */		
	case 1:
		buf = (char*)malloc(sizeof(bool));
		if (buf == NULL) {
			LOGE("No memory");
			return NULL;
		}
		*size = sizeof(bool);
		
		if (plb->lb_start(cmd->hash) < 0) {
		    *(bool*)buf = false;
			LOGE("start statics error");
		} else {
		    *(bool*)buf = false;
		}
		break;
		
	/* 关闭LB */	
	case 2:
		buf = (char*)malloc(sizeof(bool));
		if (buf == NULL) {
			LOGE("No memory");
			return NULL;
		}
		*size = sizeof(bool);
		
		if (plb->lb_stop(cmd->hash) < 0) {
		    *(bool*)buf = false;
			LOGE("stop statics error");
		} else {
		    *(bool*)buf = true;
		}
		break;
		
	/* 获取LB状态 */	
	case 3:
		buf = (char*)malloc(2*sizeof(bool));
		if (buf == NULL) {
			LOGE("No memory");
			return NULL;
		}
		*size = 2*sizeof(bool);
	    *(bool*)buf = true;
		
		if (plb->is_lb_start(cmd->hash)) {
		    *(bool*)(buf + sizeof(bool))= true;
		} else {
		    *(bool*)(buf + sizeof(bool))= false;
		}
		break;
		
	default:
		buf = (char*)malloc(sizeof(bool));
		if (buf == NULL) {
			LOGE("No memory");
			return NULL;
		}
		*size = sizeof(bool);
	    *(bool*)buf = false;
	}
	return buf;
}


void *cmdsrv::group_lb(LB_CMD *cmd, size_t *size) {
	return NULL;
}


void cmdsrv::read(int sock, short event, void* arg) {
	cmdsrv *srv = (cmdsrv *)arg;
	void *buffer;
	size_t size;
	
	/* 接收数据 */
	buffer = srv->ev_recv(&size);
	if ((int)size <= 0) {
		/* = 0: 客户端断开连接，在这里移除读事件并且释放客户数据结构 */
		/* < 0: 出现了其它的错误，在这里关闭socket，移除事件并且释放客户数据结构 */
		delete srv;
		return;
	}
	
	/* 处理命令 */
    LB_CMD *pcmd = (LB_CMD *) buffer;
    LOGD("Get command");
    LOGD("  CMD: 0x%08x", pcmd->cmd);
    LOGD(" HASH: 0x%lx", pcmd->hash);
    LOGD("GROUP: %d", pcmd->group);
    LOGD("   IP: 0x%08x", pcmd->ip);
    LOGD(" PORT: %d", pcmd->port);
    
    size_t resp_len;
    void *resp_buf = NULL;
    if (pcmd->cmd & 0x20000000) {

	    /* 统计命令处理器 */
	    if (pcmd->cmd & 0x10000000) {
	    	resp_buf = srv->group_stat(pcmd, &resp_len);
	    } else {
	    	resp_buf = srv->company_stat(pcmd, &resp_len);
	    }
    } else {
		
		/* 负载均衡命令处理 */
	    if (pcmd->cmd & 0x10000000) {
	    	resp_buf = srv->group_lb(pcmd, &resp_len);
	    } else {
	    	resp_buf = srv->company_lb(pcmd, &resp_len);
	    }		
    }
    
    /* 发送应答消息 */
    if (resp_buf) {
	    if (srv->ev_send(0, resp_buf, resp_len) == false) {
	    	LOGE("reponse error");
	    }
	}
	
	/* 释放缓冲区 */
	srv->recv_done(buffer);
}

void cmdsrv::send_done(unsigned long int token, void *buf, size_t len) {
	/* 释放发送缓冲区 */
	free(buf);
}
