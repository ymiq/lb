#ifndef __CFG_DB_H__
#define __CFG_DB_H__

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <lb_db.h>

using namespace std;

#define CFG_GROUP_INDEX_SIZE		128

typedef struct group_info {
	int handle;
	int groupid;
	unsigned int master;
	unsigned int slave;
	int port;
} group_info;


typedef struct group_index {
	group_info  items[CFG_GROUP_INDEX_SIZE];
	group_index *next;
} group_index;


class cfg_db : public lb_db {
public:
	~cfg_db();
	cfg_db(const char *ip = "localhost", unsigned short port = 3306, const char *db_name = "lb_db");
			
	int init_lb_table(clb_tbl *plb, clb_grp *pgrp);
	int init_stat_table(stat_tbl *pstat);
		
protected:
	
private:
	group_index *grp_idx;

	int opensock(unsigned int master, int port);
	int getsock(int groupid, unsigned int master, int qport);
};


#endif /* __CFG_DB_H__ */
