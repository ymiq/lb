#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <openssl/md5.h>
#include <hash_alg.h>

using namespace std; 

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

