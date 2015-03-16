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


unsigned long company_hash(const char *company) {
	MD5_CTX ctx;
	unsigned char md5[16];
	unsigned long hash = 0;
	
	MD5_Init(&ctx);
	MD5_Update(&ctx, company, strlen(company));
	MD5_Final(md5, &ctx);
	
	memcpy(&hash, md5, sizeof(hash));
	
	return hash;
}


unsigned long string_hash(const char *string) {
	MD5_CTX ctx;
	unsigned char md5[16];
	unsigned long hash = 0;
	
	MD5_Init(&ctx);
	MD5_Update(&ctx, string, strlen(string));
	MD5_Final(md5, &ctx);
	
	memcpy(&hash, md5, sizeof(hash));
	
	return hash;
}


unsigned long pointer_hash(void *ptr) {
	return hash_64((unsigned long)ptr, 64);
}
