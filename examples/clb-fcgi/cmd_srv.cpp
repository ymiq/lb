
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>

#include <unistd.h>
#include <log.h>
#include <stat_man.h>
#include <clb_tbl.h>
#include <qao/cctl_req.h>
#include <qao/cctl_rep.h>
#include <clb_grp.h>
#include "cmd_srv.h"

using namespace std;

cmd_srv::cmd_srv(int fd, struct event_base* base) : evsock(fd, base) {
	pstat = stat_man::get_inst();
	plb = clb_tbl::get_inst();
	pgrp = clb_grp::get_inst();
}


cmd_srv::~cmd_srv() {
}


qao_base *cmd_srv::company_stat(cctl_req &req) {
	list<unsigned long>::iterator it;
	CCTL_REP2 resp;
	cctl_rep2 *ret = new cctl_rep2;
	
	memset(&resp, 0, sizeof(CCTL_REP2));
	unsigned int command = req.command & 0x0fffffff;
	switch(command) {
		
	/* 开启统计 */		
	case 0x01:
		for (it=req.hash_list.begin(); it!=req.hash_list.end(); it++) {
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
		for (it=req.hash_list.begin(); it!=req.hash_list.end(); it++) {
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
		for (it=req.hash_list.begin(); it!=req.hash_list.end(); it++) {
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
		for (it=req.hash_list.begin(); it!=req.hash_list.end(); it++) {
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


qao_base *cmd_srv::group_stat(cctl_req &req) {
	return NULL;
}


qao_base *cmd_srv::company_lb(cctl_req &req) {
	list<unsigned long>::iterator it;
	CCTL_REP0 resp = {0};
	cctl_rep0 *ret = new cctl_rep0;
	
	memset(&resp, 0, sizeof(CCTL_REP0));
	unsigned int command = req.command & 0x0fffffff;
	switch(command) {
		
	/* 开启服务 */		
	case 0x01:
		for (it=req.hash_list.begin(); it!=req.hash_list.end(); it++) {
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
		for (it=req.hash_list.begin(); it!=req.hash_list.end(); it++) {
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
		for (it=req.hash_list.begin(); it!=req.hash_list.end(); it++) {
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
		for (it=req.hash_list.begin(); it!=req.hash_list.end(); it++) {
			unsigned long hash = *it;
			clb_grp_info grp_info;
			grp_info.group = req.dst_groupid;
			grp_info.handle = -1;
			grp_info.ip = req.ip;
			grp_info.port = req.port;
			grp_info.lb_status = 1;
			grp_info.stat_status = 0;
			
			/* 创建/填充Group表 */
			if (pgrp->create(grp_info, hash) == NULL) {
				resp.success = false;
			} else {
				lbsrv_info info;

				/* 填充clb hash表 */
				info.hash = hash;
				info.group = req.dst_groupid;
				info.lb_status = 1;
				info.stat_status= 0;
				info.handle = grp_info.handle;
				if (plb->create(info) < 0) {
					resp.success = false;
					pgrp->remove(req.dst_groupid, hash);
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
		for (it=req.hash_list.begin(); it!=req.hash_list.end(); it++) {
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
		unsigned int group = req.dst_groupid;
		if (group == -1u) {
			ret->success = false;
			break;
		}
		int handle = pgrp->get_handle(group);
		if (handle < 0) {
			ret->success = false;
			break;
		}
		for (it=req.hash_list.begin(); it!=req.hash_list.end(); it++) {
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


qao_base *cmd_srv::group_lb(cctl_req &req) {
	list<unsigned int>::iterator it;
	CCTL_REP1 resp = {0};
	cctl_rep1 *ret = new cctl_rep1;
	
	memset(&resp, 0, sizeof(CCTL_REP1));
	unsigned int command = req.command & 0x0fffffff;
	switch(command) {
		
	/* 开启服务 */		
	case 0x01:
		for (it=req.group_list.begin(); it!=req.group_list.end(); it++) {
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
		for (it=req.group_list.begin(); it!=req.group_list.end(); it++) {
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
	case 0x04:
		for (it=req.group_list.begin(); it!=req.group_list.end(); it++) {
			unsigned int group = *it;
			resp.group = group;
			
			clb_grp_info *grp_info = pgrp->find(group);
			if (grp_info == NULL) {
				resp.ip = 0;
				resp.port = 0;
				resp.lb_status = 0;
				resp.stat_status = 0;
				resp.handle = -1;
				resp.success = false;
			} else {
				resp.ip = grp_info->ip;
				resp.port = grp_info->port;
				resp.lb_status = grp_info->lb_status;
				resp.stat_status = grp_info->stat_status;
				resp.handle = grp_info->handle;
				resp.success = true;
			}
			
			ret->resp_list.push_back(resp);
		}
		ret->success = true;
		break;
		
	/* 新增组 */	
	case 0x08:
	{
		clb_grp_info grp_info;
		grp_info.group = resp.group = req.dst_groupid;
		grp_info.ip = resp.ip = req.ip;
		grp_info.port = resp.port = req.port;
		grp_info.lb_status = 1;
		grp_info.stat_status = 0;
		grp_info.handle = -1;
		
		clb_grp_info *ret_grp_info = pgrp->create(grp_info);
		if (ret_grp_info == NULL) {
			resp.success = false;
			resp.handle = -1;
		} else {
			resp.success = true;
			resp.handle = ret_grp_info->handle;
		}
		ret->success = resp.success;
		ret->resp_list.push_back(resp);
		break;
	}
		
	/* 删除组 */
	case 0x10:
		for (it=req.group_list.begin(); it!=req.group_list.end(); it++) {
			unsigned int group = *it;
			resp.group = group;
			
			/* 删除组信息, 同时删除当前组下所有公司CLB信息 */
			pgrp->remove(group);
			ret->resp_list.push_back(resp);
		}
		ret->success = true;
 		break;

	/* 切换组 */
	case 0x20:
	{
		unsigned int dst_group = req.dst_groupid;
		unsigned int src_group = req.src_groupid;
		
		clb_grp_info *grp_info = pgrp->move(src_group, dst_group);
		if (grp_info == NULL) {
			resp.ip = 0;
			resp.port = 0;
			resp.lb_status = 0;
			resp.stat_status = 0;
			resp.handle = -1;
			resp.success = false;
		} else {
			resp.ip = grp_info->ip;
			resp.port = grp_info->port;
			resp.lb_status = grp_info->lb_status;
			resp.stat_status = grp_info->stat_status;
			resp.handle = grp_info->handle;
			resp.success = true;
		}
		ret->resp_list.push_back(resp);
		break;
	}

	default:
		ret->success = false;
	}
	return ret;
}


void cmd_srv::read(int sock, short event, void* arg) {
	cmd_srv *srv = (cmd_srv *)arg;
	void *buffer;
	size_t size = 0;
	bool fragment = false;
	
	/* 接收数据 */
	buffer = srv->ev_recv(size, fragment);
	if ((int)size <= 0) {
		/* = 0: 客户端断开连接，在这里移除读事件并且释放客户数据结构 */
		/* < 0: 出现了其它的错误，在这里关闭socket，移除事件并且释放客户数据结构 */
		delete srv;
		return;
	} else if (fragment) {
		return;
	} else if (size <= sizeof(serial_data)) {
		/* 释放缓冲区 */
		srv->recv_done(buffer);
		return;
	}
//	LOGI("RECV(%d): %s", size, (const char*)buffer + sizeof(serial_data));
	
	/* 处理命令 */
	try {
		cctl_req req((const char*)buffer, size);
		
		unsigned int command = req.command;
		
	    qao_base *rep = NULL;
	    if (command & 0x20000000) {
	
		    /* 统计命令处理器 */
		    if (command & 0x10000000) {
		    	rep = srv->group_stat(req);
		    } else {
		    	rep = srv->company_stat(req);
		    }
	    } else {
			
			/* 负载均衡命令处理 */
		    if (command & 0x10000000) {
		    	rep = srv->group_lb(req);
		    } else {
		    	rep = srv->company_lb(req);
		    }		
	    }
	    
	    /* 发送QAO对象 */
	    if (rep) {
		    if (srv->ev_send(rep) == false) {
		    	LOGE("reponse error");
			}
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


void cmd_srv::send_done(void *buf, size_t len, bool send_ok) {

}


void cmd_srv::send_done(qao_base *rep, bool send_ok) {
	/* 销毁对象 */
	delete rep;
}

