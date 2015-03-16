

#ifndef __HASH_ALG_H__
#define __HASH_ALG_H__

extern "C" {
	
unsigned long company_hash(const char *company);
unsigned long string_hash(const char *string);
unsigned long pointer_hash(void *ptr);

}

#endif /* __HASH_ALG_H__ */
