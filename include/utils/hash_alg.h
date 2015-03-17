

#ifndef __HASH_ALG_H__
#define __HASH_ALG_H__

extern "C" {
	
unsigned long pointer_hash(void *ptr);
unsigned long string_hash(const char *string);
unsigned long data_hash(const char *data, int len);

unsigned long company_hash(const char *company);
}

#endif /* __HASH_ALG_H__ */
