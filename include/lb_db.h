

#ifndef __LB_DB_H__
#define __LB_DB_H__

#include <cstdlib>
#include <cstddef>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <mysql/mysql.h>

#include <config.h>

using namespace std; 


class lb_db {
public:
	lb_db(const char *ip = "localhost", unsigned short port = 3306, const char *db_name = "lb_db");
	~lb_db();
	
	int db_create(void);
	int db_dump(void);
	unsigned long check_company(const char *company);
	bool check_groupid(unsigned int groupid);

protected:
	MYSQL mysql;
	
private:
	unsigned long compute_hash(const char *company);
		
};

#endif /* __LB_DB_H__ */
