

#ifndef _CONFIG_H__
#define _CONFIG_H__

#define CFG_QAO_TRACE
// #define CFG_QAO_DUMP	

/* 客户端均衡表和统计表索引 */
#define CFG_INDEX_SIZE	0x10000
#define CFG_ITEMS_SIZE	7
#define CFG_CACHE_ALIGN	64
#define CFG_MAX_THREADS	64

/* 使用单线程模式还是多线程模式 */
#define CFG_CLBSRV_MULTI_THREAD		0
#define CFG_HUSB_MULTI_THREAD		0
#define CFG_CLBFCGI_MULTI_THREAD	0

/* Memory Barrier */
#define wmb() 			__sync_synchronize()
#define rmb() 			__sync_synchronize()
#define mb() 			__sync_synchronize()

#endif /* _CONFIG_H__ */
