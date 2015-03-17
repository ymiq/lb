/* 下面包括输出文件和库文件头。*/
#include <stdio.h>
#include <string.h>
#include <expat.h>


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


/* 创建一个缓冲区。*/
char xml[] = {
	"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
	"<xml>"
	"<TE><feed version=\"2.0\" ctxt-id=\"9212\" template-id=\"default\" feed-type=\"ftti\"></feed></TE>"
	"<ToUserName><![CDATA[toUser]]></ToUserName>"
	"<FromUserName><![CDATA[fromUser]]></FromUserName>"
	"<CreateTime>***********</CreateTime>"
	"<MsgType><![CDATA[text]]></MsgType>"
	"<Content><![CDATA[this is a test]]></Content>"
	"<MsgId>1234567890123456</MsgId>"
	"</xml>"
};

int depth;


/* 下面定义一个XML元素开始处理的函数。*/
static void start(void *data, const char *tag, const char **attr) {
	int depth = *(int*)data;
	
	for (int i = 0; i < depth; i++) {
		printf("	");
	}

	printf("<%s>", tag);
	for (int i = 0; attr[i]; i += 2) {
		printf(" %s='%s'", attr[i], attr[i + 1]);
	}

	printf("\n");
	*(int*)data = depth + 1;
} 


/* 下面定义一个XML元素结束调用的函数。*/
static void end(void *data, const char *tag) {
	int *pdepth = (int*)data;
	(*pdepth)--;
}


static void data_handle(void *data, const char *s, int len) {
	char buf[256];
	memcpy(buf, s, len);
	buf[len] = '\0';
	printf(" %s", s);
}


/* 程序入口点。*/
int main(int argc, char **argv) {

	printf("%lx hash(0)\n", hash_64(0, 64));
	printf("%lx hash(1)\n", hash_64(1, 64));
	printf("%lx hash(-1)\n", hash_64(-1, 64));
	return 0;

	/* 创建一个XML分析器。*/
	XML_Parser parser = XML_ParserCreate(NULL);

	/* 下面判断是否创建XML分析器失败。*/
	if (! parser) {
		fprintf(stderr, "Couldn't allocate memory for parser\n");
		exit(-1);
	}


	/* 下面设置每个XML元素出现和结束的处理函数。这里设置start为元素开始处理函数，end元素结束处理函数。*/
	XML_SetUserData(parser, &depth);
	XML_SetElementHandler(parser, start, end);
	XML_SetCharacterDataHandler(parser, data_handle);

	/* 循环分析所有XML文件。*/
	int len = strlen(xml);

	/* 调用库函数XML_Parse来分析缓冲区Buff里的XML数据。*/
	if (!XML_Parse(parser, xml, len, 1)) {
		fprintf(stderr, "Parse error at line %ld:\n %s\n",
				XML_GetCurrentLineNumber(parser),
				XML_ErrorString(XML_GetErrorCode(parser)));
		exit(-1);
	}
	
	XML_ParserFree(parser);
	return 0;
}



 