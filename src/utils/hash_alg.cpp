#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <openssl/md5.h>
#include <utils/hash_alg.h>

/* 2^31 + 2^29 - 2^25 + 2^22 - 2^19 - 2^16 + 1 */
#define GOLDEN_RATIO_PRIME_32 0x9e370001UL
/*  2^63 + 2^61 - 2^57 + 2^54 - 2^51 - 2^18 + 1 */
#define GOLDEN_RATIO_PRIME_64 0x9e37fffffffc0001UL

static inline unsigned long hash_64(unsigned long val, unsigned int bits)
{
	unsigned long hash = val;

	/*  Sigh, gcc can't optimise this alone like it does for 32 bits. */
	unsigned long n = hash;
	n <<= 18;
	hash -= n;
	n <<= 33;
	hash -= n;
	n <<= 3;
	hash += n;
	n <<= 3;
	hash -= n;
	n <<= 4;
	hash += n;
	n <<= 2;
	hash += n;

	/* High bits are more random, so use them. */
	return hash >> (64 - bits);
}

static inline unsigned int hash_32(unsigned int val, unsigned int bits)
{
	/* On some cpus multiply is faster, on others gcc will do shifts */
	unsigned int hash = val * GOLDEN_RATIO_PRIME_32;

	/* High bits are more random, so use them. */
	return hash >> (32 - bits);
}


unsigned long pointer_hash(void *ptr) {
	unsigned long ret = hash_64((unsigned long)ptr, 64);
	if ((ret == 0) || (ret == 1) || (ret == -1ul)) {
		ret = GOLDEN_RATIO_PRIME_64;
	}
	return ret;
}


unsigned long string_hash(const char *string) {
	MD5_CTX ctx;
	unsigned long hash[4];
	
	MD5_Init(&ctx);
	MD5_Update(&ctx, string, strlen(string));
	MD5_Final((unsigned char*)hash, &ctx);
	
	unsigned long ret = hash[0];
	if ((ret == 0) || (ret == 1) || (ret == -1ul)) {
		ret = GOLDEN_RATIO_PRIME_64;
	}
	return ret;
}


unsigned long data_hash(const char *data, int len) {
	MD5_CTX ctx;
	unsigned long hash[4];
	
	MD5_Init(&ctx);
	MD5_Update(&ctx, data, len);
	MD5_Final((unsigned char*)hash, &ctx);
	
	unsigned long ret = hash[0];
	if ((ret == 0) || (ret == 1) || (ret == -1ul)) {
		ret = GOLDEN_RATIO_PRIME_64;
	}
	return ret;
}


unsigned long company_hash(const char *company) {
	return string_hash(company);
}

