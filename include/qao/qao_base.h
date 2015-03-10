

#ifndef __QAO_BASE_H__
#define __QAO_BASE_H__

#include <cstdlib>
#include <cstddef>
#include <string>
#include <config.h>
#include <stat_obj.h>
#include <hash_tbl.h>

using namespace std; 

/* 数据结构中数据为网络字节序 */
typedef struct serial_data {
	unsigned long token;
	unsigned int length;
	unsigned char type;
	unsigned char version;
	unsigned short packet_len;
	unsigned char data[0];
}__attribute__((packed)) serial_data;


#define	QAO_CLB_CTL_REQ		0x10
#define	QAO_CLB_CTL_REP0	0x11
#define	QAO_CLB_CTL_REP1	0x12
#define	QAO_CLB_CTL_REP2	0x13
#define	QAO_CLB_CTL_REP3	0x14

class qao_base {
public:
	qao_base() : qao_token(0), qao_type(0), qao_version(0), qao_qos(0) {};
	virtual ~qao_base() {};
	
	virtual void *serialization(size_t &len, unsigned long token) = 0;
	virtual void *serialization(size_t &len) = 0;
	
	int set_qos(int qos) {int ret = qao_qos; qao_qos = qos & 0x3; return ret;};
	int get_qos(void) {return qao_qos;};

protected:
	unsigned long qao_token;			/* Token */
	unsigned int qao_type;				/* 对象类型 */
	unsigned int qao_version;			/* 对象版本 */
	unsigned int qao_qos;				/* QoS */
private:
};

#endif /* __QAO_BASE_H__ */
