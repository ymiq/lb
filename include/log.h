

#ifndef _LOGGER_H__
#define _LOGGER_H__

#include <cstdarg>
#include <syslog.h>

extern "C" {

void log_open(const char *name);
void log_printf(int prio, const char* fmt, ...);
void log_printf_pos(int prio, const char *file, 
			const char *func, int line, const char* fmt, ...);
void log_close(void);

void log_console(const char* fmt, ...);
void log_trace(const char *file, const char *func, int line, const char* fmt, ...);

#define LOG_OPEN(name)		log_open(name);
#define LOGE(...)			log_printf_pos(LOG_ERR, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOGW(...)			log_printf(LOG_WARNING, __VA_ARGS__)
#define LOGI(...)			log_printf(LOG_INFO, __VA_ARGS__)
#define LOGD(...)			log_printf(LOG_DEBUG, __VA_ARGS__)
#define LOG_CLOSE()			log_close()

/* 向终端输出消息 */
#define LOGC(...)			log_console(__VA_ARGS__)

/* 调试断点 */
#define LOGT(...)			log_trace(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

}

#endif /* _LOGGER_H__ */
