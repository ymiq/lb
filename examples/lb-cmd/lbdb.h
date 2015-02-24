

#ifndef _LBDB_H__
#define _LBDB_H__

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <mysql/mysql.h>

using namespace std; 

class lbdb {
public:
	lbdb();
	~lbdb();
	
	int db_dump(void);
	int db_create(void);
	
	uint64_t get_hash(const char *company);
	bool check_groupid(int groupid);

protected:
	
private:
	MYSQL mysql;
};

#endif /* _LBDB_H__ */
