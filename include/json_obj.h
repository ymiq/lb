

#ifndef __JSON_OBJECT_H__
#define __JSON_OBJECT_H__

#include <cstdlib>
#include <cstddef>
#include <string>
#include <list>
#include <json/json.h>

using namespace std; 

class json_object {
public:
	virtual ~json_object() {};
	json_object() {};
	json_object(const char *str) {};
	
	virtual string serialization(void) = 0;
	
protected:
	
private:
};

#endif /* __JSON_OBJECT_H__ */
