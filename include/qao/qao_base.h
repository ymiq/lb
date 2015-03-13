

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
	unsigned int length;		/* 高2位表示分片信息; 低30位表示长度或者偏移 */
								/* 00-无分片; 低30位表示总长度（含serial_data) */
								/* 01-第一个分片包; 低30位表示总长度（含serial_data) */
								/* 10-中间分片包; 低30位表示数据起始偏移 */
								/* 11-最后一个分片包; 低30位表示数据起始偏移  */
	unsigned char type;
	unsigned char version;		/* 高2位表示处理优先级Qos */
	unsigned short datalen;		/* 数据长度，不包含serial_data长度 */
	unsigned char data[0];
}__attribute__((packed)) serial_data;


/* CLB相关对象 */
#define	QAO_CCTL_REQ		0x10
#define	QAO_CCTL_REP0		0x11
#define	QAO_CCTL_REP1		0x12
#define	QAO_CCTL_REP2		0x13
#define	QAO_CCTL_REP3		0x14
#define	QAO_CDAT_WX			0x18

class qao_base {
public:
	qao_base() : qao_token(0), qao_type(0), qao_version(0), qao_qos(3) {};
	virtual ~qao_base() {};
	
	virtual char *serialization(size_t &len) = 0;
	virtual void dump(void) = 0;					/* 方便调试、跟踪分析。该方法可以不实现 */
	
	int set_qos(int qos);
	int get_qos(void);
	void init(int type, int version, int qos);
	void init(serial_data *header);
	void serial_header(serial_data *header);
	void serial_header(serial_data *header, int len_off, int datalen);
	void serial_header(serial_data *header, int len_off, int datalen, int fragment);

protected:
	unsigned long qao_token;	/* Token */
	unsigned int qao_type;		/* 对象类型 */
	unsigned int qao_version;	/* 对象版本 */
	unsigned int qao_qos;		/* QoS */
	
private:
	static unsigned int seqno;		
};


#endif /* __QAO_BASE_H__ */
