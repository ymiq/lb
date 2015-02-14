

#ifndef _LBDB_H__
#define _LBDB_H__

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <mysql/mysql.h>

#include "config.h"

using namespace std; 

class lbdb {
public:
	lbdb();
	~lbdb();
	
	int dump(void);
	int create(void);

protected:
	
private:
	MYSQL mysql;
};

#endif /* _LBDB_H__ */
