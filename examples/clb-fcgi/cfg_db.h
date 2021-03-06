﻿#ifndef __CFG_DB_H__
#define __CFG_DB_H__

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <utils/lb_db.h>
#include <stat/stat_tbl_base.h>

using namespace std;


class cfg_db : public lb_db {
public:
	~cfg_db();
	cfg_db(const char *ip = "localhost", unsigned short port = 3306, const char *db_name = "lb_db");
			
	int init_lb_table(clb_tbl *plb, clb_grp *pgrp);
	int init_stat_table(stat_tbl_base *pstat);
		
protected:
	
private:
};


#endif /* __CFG_DB_H__ */
