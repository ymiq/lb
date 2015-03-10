
#include <iostream>
#include <cstdarg>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <syslog.h>
#include <log.h>


using namespace std;

#define LOG_BUF_SIZE		2048

static bool console = false;

void log_open_console(const char *name) {
	console = true;
}


void log_open(const char *name) {
	openlog(name, LOG_ODELAY | LOG_PID, LOG_USER);
}


void log_printf(int prio, const char* fmt, ...) {
    va_list ap;
    char buf[LOG_BUF_SIZE];

    va_start(ap, fmt);
    vsnprintf(buf, LOG_BUF_SIZE, fmt, ap);
    va_end(ap);
    if (console) {
	    cout << buf << endl;
    } else {
    	syslog(prio, buf);
    }
}


void log_printf_pos(int prio, const char *file, 
			const char *func, int line, const char* fmt, ...) {
    va_list ap;
    char prefix[LOG_BUF_SIZE*2];
    char buf[LOG_BUF_SIZE];
	
	snprintf(prefix, LOG_BUF_SIZE, "%s:%s:%d: ", file, func, line);
	
    va_start(ap, fmt);
    vsnprintf(buf, LOG_BUF_SIZE, fmt, ap);
    va_end(ap);
    strcat(prefix, buf);
    if (console) {
   		cout << prefix << endl;
    } else {
    	syslog(prio, prefix);
    }
}


void log_close(void) {
	if (!console) {
		closelog();
	}
}


void log_console(const char* fmt, ...) {
    va_list ap;
    char buf[LOG_BUF_SIZE];
	
    va_start(ap, fmt);
    vsnprintf(buf, LOG_BUF_SIZE, fmt, ap);
    va_end(ap);
    
    cout << buf << endl;
}


void log_trace(const char *file, 
			const char *func, int line, const char* fmt, ...) {
    va_list ap;
    char prefix[LOG_BUF_SIZE*2];
    char buf[LOG_BUF_SIZE];
	
	snprintf(prefix, LOG_BUF_SIZE, "<TRACE>:%s:%s:%d ", file, func, line);
	
    va_start(ap, fmt);
    vsnprintf(buf, LOG_BUF_SIZE, fmt, ap);
    va_end(ap);
    strcat(prefix, buf);
    if (console) {
	    cout << prefix << endl;
    } else {
    	syslog(LOG_INFO, prefix);    
    }
}


