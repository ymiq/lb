
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>

#include <unistd.h>
#include <log.h>
#include <stat_man.h>
#include <lb_table.h>
#include <json_obj.h>
#include <clb_cmd.h>
#include "cmdsrv.h"

using namespace std;

cmdsrv::cmdsrv(int fd, struct event_base* base):evsock(fd, base) {
	pstat = stat_man::get_inst();
	plb = lb_table::get_inst();
}


cmdsrv::~cmdsrv() {
}


clb_cmd_resp *cmdsrv::company_stat(clb_cmd &cmd) {
	list<unsigned long int>::iterator it;
	CLB_CMD_RESP100 resp;
	clb_cmd_resp100 *ret = new clb_cmd_resp100;
	
	memset(&resp, 0, sizeof(CLB_CMD_RESP100));
	unsigned int command = cmd.command & 0x0fffffff;
	switch(command) {
		
	/* 开启统计 */		
	case 1:
		for (it=cmd.hash_list.begin(); it!=cmd.hash_list.end(); it++) {
			unsigned long int hash = *it;
			resp.hash = hash;

			/* 创建统计对象 */
			if (pstat->start(hash, 1) < 0) {
				resp.success = false;
			} else {
				/* 打开统计开关 */
				if (plb->stat_start(hash) <0 ) {
					resp.success = false;
				} else {
					resp.success = true;
				}
			}
			ret->resp_list.push_back(resp);
		}
		break;
		
	/* 关闭统计 */
	case 2:
		for (it=cmd.hash_list.begin(); it!=cmd.hash_list.end(); it++) {
			unsigned long int hash = *it;
			resp.hash = hash;

			/* 关闭统计开关 */
			resp.success = true;
			if (plb->stat_stop(hash) < 0) {
				resp.success = false;
			}
			
			/* 销毁统计对象 */
			if (pstat->stop(hash) <0 ) {
				resp.success = false;
			}
			
			ret->resp_list.push_back(resp);
		}
 		break;
		
	/* 获取统计信息 */	
	case 4:
		for (it=cmd.hash_list.begin(); it!=cmd.hash_list.end(); it++) {
			unsigned long int hash = *it;
			resp.hash = hash;
			
			if (pstat->read(hash, &resp.info, &resp.tm) < 0) {
				resp.success = false;
			} else {
				resp.success = true;
			}
			
			ret->resp_list.push_back(resp);
		}
		
	default:
		ret->success = false;
	}
	return ret;
}


clb_cmd_resp *cmdsrv::group_stat(clb_cmd &cmd) {
	return NULL;
}


clb_cmd_resp *cmdsrv::company_lb(clb_cmd &cmd) {
	list<unsigned long int>::iterator it;
	CLB_CMD_RESP0 resp = {0};
	clb_cmd_resp0 *ret = new clb_cmd_resp0;
	
	memset(&resp, 0, sizeof(CLB_CMD_RESP0));
	unsigned int command = cmd.command & 0x0fffffff;
	switch(command) {
		
	/* 开启服务 */		
	case 1:
		for (it=cmd.hash_list.begin(); it!=cmd.hash_list.end(); it++) {
			unsigned long int hash = *it;
			resp.hash = hash;
			
			if (plb->lb_start(hash) < 0) {
				resp.success = false;
			} else {
				resp.success = true;
			}
			ret->resp_list.push_back(resp);
		}
		break;
		
	/* 关闭服务 */
	case 2:
		for (it=cmd.hash_list.begin(); it!=cmd.hash_list.end(); it++) {
			unsigned long int hash = *it;
			resp.hash = hash;

			if (plb->lb_stop(hash) < 0) {
				resp.success = false;
			} else {
				resp.success = true;
			}
			ret->resp_list.push_back(resp);
		}
 		break;
		
	/* 获取服务信息 */	
	case 4:
		for (it=cmd.hash_list.begin(); it!=cmd.hash_list.end(); it++) {
			unsigned long int hash = *it;
			lbsrv_info info;
			resp.hash = hash;

			if (plb->lb_info(hash, &info) < 0) {
				resp.success = false;
			} else {
				resp.success = true;
				resp.info = info;
			}
			
			ret->resp_list.push_back(resp);
		}
		
	default:
		ret->success = false;
	}
	return ret;
}


clb_cmd_resp *cmdsrv::group_lb(clb_cmd &cmd) {
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
	// LOGI((const char*)buffer);
	
	/* 处理命令 */
	try {
		clb_cmd cmd((const char*)buffer);
		
		unsigned int command = cmd.command;
		
	    clb_cmd_resp *pjson = NULL;
	    if (command & 0x20000000) {
	
		    /* 统计命令处理器 */
		    if (command & 0x10000000) {
		    	pjson = srv->group_stat(cmd);
		    } else {
		    	pjson = srv->company_stat(cmd);
		    }
	    } else {
			
			/* 负载均衡命令处理 */
		    if (command & 0x10000000) {
		    	pjson = srv->group_lb(cmd);
		    } else {
		    	pjson = srv->company_lb(cmd);
		    }		
	    }
	    
	    if (pjson) {
		    /* 发送应答消息 */
		    string resp_str = pjson->serialization();
		    int len = resp_str.length();
		    if (len > 0) {
			    char *obuf = (char*)malloc(len + 1);
			    if (obuf) {
				    strcpy(obuf, resp_str.c_str());
				    if (srv->ev_send(0, obuf, len + 1) == false) {
				    	LOGE("reponse error");
				    }
			    }
			}
			
			/* 销毁对象 */
			delete pjson;
		}
	} catch (const char *msg) {
    	LOGE("Command json error");
		/* 释放缓冲区 */
		srv->recv_done(buffer);
		return;
	}
	
	/* 释放缓冲区 */
	srv->recv_done(buffer);
}

void cmdsrv::send_done(unsigned long int token, void *buf, size_t len) {
	/* 释放发送缓冲区 */
	free(buf);
}
