

#ifndef _LBDB_H__
#define _LBDB_H__

#include <cstdlib>
#include <cstddef>
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
	
	unsigned long int get_hash(const char *company);
	bool check_groupid(int groupid);

protected:
	
private:
	MYSQL mysql;
	
	unsigned long int compute_hash(const char *company);
};

#endif /* _LBDB_H__ */
