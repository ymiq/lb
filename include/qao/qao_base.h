

#ifndef __QAO_BASE_H__
#define __QAO_BASE_H__

#include <cstdlib>
#include <cstddef>
#include <string>
#include <config.h>
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
	unsigned int checksum;		/* 头信息校验值 */	
	unsigned char data[0];
}__attribute__((packed)) serial_data;


/* CLB相关对象 */
#define	QAO_CCTL_REQ		0x10
#define	QAO_CCTL_REP0		0x11
#define	QAO_CCTL_REP1		0x12
#define	QAO_CCTL_REP2		0x13
#define	QAO_CCTL_REP3		0x14
#define	QAO_CDAT_WX			0x18

#define QAO_QUESTION		0x20
#define QAO_ANSWER			0x30
#define QAO_CANDIDATE		0x40

#define QAO_SCLNT_DECL		0x80

class qao_base {
public:
	qao_base();
	virtual ~qao_base() {};
	
	/* 序列化函数，序列化对象，用于在服务器之间传递 */
	virtual char *serialization(size_t &len) = 0;
	
	/* 持久化函数，用于把对象保存到数据库 */
	virtual int persistence(void);

	/* 方便调试、跟踪分析。*/
	virtual void dump(void) {};
	virtual void dump(const char *fmt, ...) {};

#ifdef CFG_QAO_TRACE	
	virtual void trace(const char *fmt, ...);
	virtual string &serial_trace(void);
	virtual void init(string &trace);
	virtual void dump_trace(void);
#endif
	
	/* 基本属性操作 */
	int set_qos(int qos);
	int get_qos(void);
	int set_type(int type);
	int get_type(void);
	unsigned long get_token(void);

	/* 类型初始化 */
	void init(int type, int version, int qos);
	void init(serial_data *header);
	void init(serial_data *header, int type);
	
	/* 序列化辅助函数，用于生成序列化头信息 */
	void serial_header(serial_data *header);
	void serial_header(serial_data *header, int len_off, int datalen);
	void serial_header(serial_data *header, int len_off, int datalen, int partition);
	
	bool serial_header_check(serial_data *header);

protected:
	unsigned long qao_token;	/* Token */
	unsigned int qao_type;		/* 对象类型 */
	unsigned int qao_version;	/* 对象版本 */
	unsigned int qao_qos;		/* QoS */
	
private:
	static unsigned int seqno;
#ifdef CFG_QAO_TRACE	
	string track;
#endif
};


#endif /* __QAO_BASE_H__ */
