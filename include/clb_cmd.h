

#ifndef __CLB_CMD_H__
#define __CLB_CMD_H__

#include <cstdlib>
#include <cstddef>
#include <string>
#include <list>
#include <json/json.h>

using namespace std; 


class clb_cmd {
public:
	clb_cmd();
	clb_cmd(string &string) {};
	~clb_cmd();
	
	string *serialization(void);

public:
	list<unsigned long int> hash_list;
	list<unsigned int> group_list;
	unsigned int command;
	unsigned int ip;
	unsigned short port;

protected:
	
private:
		
};

#endif /* __CLB_CMD_H__ */
