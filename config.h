

#ifndef _CONFIG_H__
#define _CONFIG_H__

#define CFG_INDEX_SIZE	0x10000
#define CFG_ITEMS_SIZE	7
#define CFG_CACHE_ALIGN	64
#define CFG_MAX_THREADS	64

#define mb() 			__sync_synchronize()

#endif /* _CONFIG_H__ */
