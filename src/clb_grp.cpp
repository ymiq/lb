#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <exception>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/unistd.h>
#include <arpa/inet.h>
#include <log.h>
#include <clb_grp.h>
#include <clb_tbl.h>
#include <stat_tbl.h>

clb_grp::clb_grp() {
	pclb = clb_tbl::get_inst();
}


clb_grp::~clb_grp() {
	hash_tbl<clb_grp_info, 64>::it it;
	for (it=table.begin(); it!=table.end(); ++it) {
		clb_grp_info *grp_info = *it;
		if (grp_info) {
			if (grp_info->phashs) {
				delete grp_info->phashs;
			}
		}
	}
}


unsigned long clb_grp::group_hash(unsigned int group) {
	return (unsigned long)(group + 1);
}


void clb_grp::remove(unsigned int group) {
	unsigned long ghash = group_hash(group);
	clb_grp_info *grp_info = table.find(ghash);
	if (grp_info) {
		/* 删除当前组CLB信息 */
		GROUP_HASH_ARRAY::it it;
			
		for (it=grp_info->phashs->begin(); it!=grp_info->phashs->end(); ++it) {
			unsigned long hash = *it;
			if (hash) {
				pclb->remove(hash);
			}
		}
		
		table.remove(ghash);
	}
}


void clb_grp::remove(unsigned int group, unsigned long hash) {
	clb_grp_info *grp_info = table.find(group_hash(group));
	if (grp_info && grp_info->phashs) {

		/* 删除Group登记信息 */
		grp_info->phashs->remove(hash);
	}
}


clb_grp_info *clb_grp::move(unsigned int src_group, unsigned int dst_group) {
	unsigned long src_ghash = group_hash(src_group);
	unsigned long dst_ghash = group_hash(dst_group);
	clb_grp_info *src_grp_info = table.find(src_ghash);	
	clb_grp_info *dst_grp_info = table.find(dst_ghash);
	if (src_grp_info && dst_grp_info) {
		int dst_handle = dst_grp_info->handle;
		
		/* 切换当前组CLB信息 */
		GROUP_HASH_ARRAY::it it;
			
		for (it=src_grp_info->phashs->begin(); it!=src_grp_info->phashs->end(); ++it) {
			unsigned long hash = *it;
			if (hash) {
				if (dst_grp_info->phashs->add(hash)) {
					pclb->lb_switch(hash, dst_group, dst_handle);
				}				
			}
		}
		return dst_grp_info;
	}
	return NULL;
}


clb_grp_info *clb_grp::find(unsigned int group) {
	return table.find(group_hash(group));
}


int clb_grp::get_handle(unsigned int group) {
	clb_grp_info *grp_info;
	grp_info = table.find(group_hash(group));
	if (grp_info == NULL) {
		return -1;
	} else {
		return grp_info->handle;
	}
}


int clb_grp::open_sock(unsigned int ip, unsigned short port) {
	int sockfd;
	struct sockaddr_in serv_addr;
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	/* 设置连接目的地址 */
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr.s_addr = htonl(ip);
 
	/* 发送连接请求 */
	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) < 0) {
		LOGE("connet to %x, %d failed", ip, port);
		return -1;
	}
	return sockfd;
}


clb_grp_info *clb_grp::create(clb_grp_info &info) {
	unsigned int group = info.group;
	unsigned long ghash;
	clb_grp_info *grp_info;
	
	ghash = group_hash(group);
	grp_info = table.find(ghash);
	if (grp_info == NULL) {
		if (info.handle < 0) {
			if (!info.ip || !info.port) {
				LOGE("No ip address and port");
				return NULL;
			}
			info.handle = open_sock(info.ip, info.port);
			if (info.handle < 0) {
				return NULL;
			}
		}
		info.phashs = new GROUP_HASH_ARRAY();
		grp_info = table.update(ghash, info);
		if (grp_info == NULL) {
			delete info.phashs;
			info.phashs = NULL;
			return NULL;
		}
	}
	if (info.handle < 0) {
		info.handle = grp_info->handle;
	}
	return grp_info;
}


clb_grp_info *clb_grp::create(clb_grp_info &info, unsigned long hash) {
	clb_grp_info *grp_info = create(info);
	if (grp_info == NULL) {
		return NULL;
	}
	
	if (!grp_info->phashs->add(hash)) {
		return NULL;
	}
	return grp_info;
}


int clb_grp::lb_start(unsigned int group) {
	clb_grp_info *grp_info = table.find(group_hash(group));
	if (!grp_info) {
		return -1;
	}

	GROUP_HASH_ARRAY::it it;
	for (it=grp_info->phashs->begin(); it!=grp_info->phashs->end(); ++it) {
		unsigned long hash = *it;
		if (hash) {
			pclb->lb_start(hash);
		}
	}
	
	return 0;
}


int clb_grp::lb_stop(unsigned int group) {
	clb_grp_info *grp_info = table.find(group_hash(group));
	if (!grp_info) {
		return -1;
	}

	GROUP_HASH_ARRAY::it it;
	for (it=grp_info->phashs->begin(); it!=grp_info->phashs->end(); ++it) {
		unsigned long hash = *it;
		if (hash) {
			pclb->lb_stop(hash);
		}
	}
	return 0;
}



int clb_grp::stat_start(unsigned int group) {
	clb_grp_info *grp_info = table.find(group_hash(group));
	if (!grp_info) {
		return -1;
	}

	GROUP_HASH_ARRAY::it it;
	for (it=grp_info->phashs->begin(); it!=grp_info->phashs->end(); ++it) {
		unsigned long hash = *it;
		if (hash) {
			pclb->stat_start(hash);
		}
	}
	
	return 0;
}


int clb_grp::stat_stop(unsigned int group) {
	clb_grp_info *grp_info = table.find(group_hash(group));
	if (!grp_info) {
		return -1;
	}

	GROUP_HASH_ARRAY::it it;
	for (it=grp_info->phashs->begin(); it!=grp_info->phashs->end(); ++it) {
		unsigned long hash = *it;
		if (hash) {
			pclb->stat_stop(hash);
		}
	}
	
	return 0;
}


