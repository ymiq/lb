#ifndef __CMDSRV_H__
#define __CMDSRV_H__

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <pthread.h>
#include <evsock.h>

using namespace std;

class cmdsrv : public evsock {
public:
	~cmdsrv();
	cmdsrv(int fd, struct event_base* base):evsock(fd, base) {}
		
	void send_done(unsigned long int token, void *buf, size_t len);
	
	static void read(int sock, short event, void* arg);
protected:
	
private:
};


#endif /* __CMDSRV_H__ */
