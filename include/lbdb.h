

#ifndef _LBDB_H__
#define _LBDB_H__

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <mysql/mysql.h>

#include "config.h"
#include "stat_table.h"

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

class lbdb {
public:
	lbdb();
	~lbdb();
	
	int db_dump(void);
	int db_create(void);
	
	int lb_create(void);
	int stat_create(stat_table *pstat);

protected:
	
private:
	MYSQL mysql;
	group_index *grp_idx;

	int lb_opensock(unsigned int master, int port);
	int lb_getsock(int groupid, unsigned int master, int qport);
};

#endif /* _LBDB_H__ */
