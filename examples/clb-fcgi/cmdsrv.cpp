
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>

#include <unistd.h>
#include <log.h>
#include <stat_man.h>
#include <clb_tbl.h>
#include <json_obj.h>
#include <clb_cmd.h>
#include <clb_grp.h>
#include "cmdsrv.h"

using namespace std;

cmdsrv::cmdsrv(int fd, struct event_base* base):evsock(fd, base) {
	pstat = stat_man::get_inst();
	plb = clb_tbl::get_inst();
	pgrp = clb_grp::get_inst();
}


cmdsrv::~cmdsrv() {
}


clb_cmd_resp *cmdsrv::company_stat(clb_cmd &cmd) {
	list<unsigned long>::iterator it;
	CLB_CMD_RESP100 resp;
	clb_cmd_resp100 *ret = new clb_cmd_resp100;
	
	memset(&resp, 0, sizeof(CLB_CMD_RESP100));
	unsigned int command = cmd.command & 0x0fffffff;
	switch(command) {
		
	/* 开启统计 */		
	case 0x01:
		for (it=cmd.hash_list.begin(); it!=cmd.hash_list.end(); it++) {
			unsigned long hash = *it;
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
		ret->success = true;
		break;
		
	/* 关闭统计 */
	case 0x02:
		for (it=cmd.hash_list.begin(); it!=cmd.hash_list.end(); it++) {
			unsigned long hash = *it;
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
		ret->success = true;
 		break;
		
	/* 获取统计信息 */	
	case 0x04:
		for (it=cmd.hash_list.begin(); it!=cmd.hash_list.end(); it++) {
			unsigned long hash = *it;
			resp.hash = hash;
			
			if (pstat->read(hash, &resp.info, &resp.tm) < 0) {
				resp.success = false;
			} else {
				resp.success = true;
			}
			
			ret->resp_list.push_back(resp);
		}
		ret->success = true;
		break;
		
	/* 清除统计信息 */	
	case 0x08:
		for (it=cmd.hash_list.begin(); it!=cmd.hash_list.end(); it++) {
			unsigned long hash = *it;
			resp.hash = hash;
			
			if (pstat->clear(hash, 0xffffffff) < 0) {
				resp.success = false;
			} else {
				resp.success = true;
			}
			
			ret->resp_list.push_back(resp);
		}
		ret->success = true;
		break;
		
	default:
		ret->success = false;
	}
	return ret;
}


clb_cmd_resp *cmdsrv::group_stat(clb_cmd &cmd) {
	return NULL;
}


clb_cmd_resp *cmdsrv::company_lb(clb_cmd &cmd) {
	list<unsigned long>::iterator it;
	CLB_CMD_RESP0 resp = {0};
	clb_cmd_resp0 *ret = new clb_cmd_resp0;
	
	memset(&resp, 0, sizeof(CLB_CMD_RESP0));
	unsigned int command = cmd.command & 0x0fffffff;
	switch(command) {
		
	/* 开启服务 */		
	case 0x01:
		for (it=cmd.hash_list.begin(); it!=cmd.hash_list.end(); it++) {
			unsigned long hash = *it;
			resp.hash = hash;
			
			if (plb->lb_start(hash) < 0) {
				resp.success = false;
			} else {
				resp.success = true;
			}
			ret->resp_list.push_back(resp);
		}
		ret->success = true;
		break;
		
	/* 关闭服务 */
	case 0x02:
		for (it=cmd.hash_list.begin(); it!=cmd.hash_list.end(); it++) {
			unsigned long hash = *it;
			resp.hash = hash;

			if (plb->lb_stop(hash) < 0) {
				resp.success = false;
			} else {
				resp.success = true;
			}
			ret->resp_list.push_back(resp);
		}
		ret->success = true;
 		break;
		
	/* 获取服务信息 */	
	case 0x04:
		for (it=cmd.hash_list.begin(); it!=cmd.hash_list.end(); it++) {
			unsigned long hash = *it;
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
		ret->success = true;
		break;
		
	/* 新增公司 */	
	case 0x08:
		for (it=cmd.hash_list.begin(); it!=cmd.hash_list.end(); it++) {
			unsigned long hash = *it;
			clb_grp_info grp_info;
			grp_info.group = cmd.dst_groupid;
			grp_info.handle = -1;
			grp_info.ip = cmd.ip;
			grp_info.port = cmd.port;
			grp_info.lb_status = 1;
			grp_info.stat_status = 0;
			
			/* 创建/填充Group表 */
			if (pgrp->create(grp_info, hash) < 0) {
				resp.success = false;
			} else {
				lbsrv_info info;
				
				/* 填充clb hash表 */
				info.hash = hash;
				info.group = cmd.dst_groupid;
				info.lb_status = 1;
				info.stat_status= 0;
				info.handle = grp_info.handle;
				if (plb->create(info) < 0) {
					resp.success = false;
					pgrp->remove(cmd.dst_groupid, hash);
				} else {
					resp.success = true;
					resp.info = info;
				}
			}		
			resp.hash = hash;	
			ret->resp_list.push_back(resp);
		}
		ret->success = true;
		break;

	/* 删除指定公司 */	
	case 0x10:
		for (it=cmd.hash_list.begin(); it!=cmd.hash_list.end(); it++) {
			unsigned long hash = *it;
			unsigned int group = plb->group_id(hash);
			resp.hash = hash;
			
			if (group != -1u) {
				plb->remove(hash);
				pgrp->remove(group, hash);
				resp.success = true;
			} else {
				resp.success = false;
			}
			
			ret->resp_list.push_back(resp);
		}
		ret->success = true;
		break;

	/* 切换均衡服务器 */	
	case 0x20:
	{
		unsigned int group = cmd.dst_groupid;
		if (group == -1u) {
			ret->success = false;
			break;
		}
		int handle = pgrp->get_handle(group);
		if (handle < 0) {
			ret->success = false;
			break;
		}
		for (it=cmd.hash_list.begin(); it!=cmd.hash_list.end(); it++) {
			unsigned long hash = *it;
			resp.hash = hash;

			if (plb->lb_switch(hash, group, handle) < 0) {
				resp.success = false;
			} else {
				resp.success = true;
			}
			ret->resp_list.push_back(resp);
		}
		ret->success = true;
		break;
	}

	default:
		ret->success = false;
	}
	return ret;
}


clb_cmd_resp *cmdsrv::group_lb(clb_cmd &cmd) {
	list<unsigned int>::iterator it;
	CLB_CMD_RESP1 resp = {0};
	clb_cmd_resp1 *ret = new clb_cmd_resp1;
	
	memset(&resp, 0, sizeof(CLB_CMD_RESP1));
	unsigned int command = cmd.command & 0x0fffffff;
	switch(command) {
		
	/* 开启服务 */		
	case 0x01:
		for (it=cmd.group_list.begin(); it!=cmd.group_list.end(); it++) {
			unsigned int group = *it;
			resp.group = group;
			
			if (pgrp->lb_start(group) < 0) {
				resp.success = false;
			} else {
				resp.success = true;
			}
			ret->resp_list.push_back(resp);
		}
		ret->success = true;
		break;
		
	/* 关闭服务 */
	case 0x02:
		for (it=cmd.group_list.begin(); it!=cmd.group_list.end(); it++) {
			unsigned int group = *it;
			resp.group = group;

			if (pgrp->lb_stop(group) < 0) {
				resp.success = false;
			} else {
				resp.success = true;
			}
			ret->resp_list.push_back(resp);
		}
		ret->success = true;
 		break;
		
	/* 获取服务信息 */	
#if 0
	case 0x04:
		for (it=cmd.group_list.begin(); it!=cmd.group_list.end(); it++) {
			unsigned int group = *it;
			resp.group = group;
			unsigned int ip;
			unsigned short port;
			unsigned int status;

			if (plb->lb_info(group, &ip, &port, &status) < 0) {
				resp.success = false;
			} else {
				resp.success = true;
				resp.ip = ip;
				resp.port = port;
				resp.status = status;
			}
			
			ret->resp_list.push_back(resp);
		}
		ret->success = true;
		break;
#endif
		
	default:
		ret->success = false;
	}
	return ret;
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
//	LOGI("RECV: %s\n", (const char*)buffer);
	
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
//				    LOGI("SEND: %s\n", obuf);
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

void cmdsrv::send_done(unsigned long token, void *buf, size_t len) {
	/* 释放发送缓冲区 */
	free(buf);
}
