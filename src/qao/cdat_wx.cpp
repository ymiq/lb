#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <utils/log.h>
#include <sys/time.h>
#include <json/json.h>
#include <json/value.h>
#include <qao/qao_base.h>
#include <qao/cdat_wx.h>
#include <utils/hash_alg.h>
#include <expat.h>

using namespace std;


cdat_wx::cdat_wx(const char *str, size_t len) {
	data = NULL;
	
	/* 创建一个XML分析器。*/
	XML_Parser parser = XML_ParserCreate(NULL);
	if (!parser) {
		throw "no memory for xml parser";
	}

	/* 下面设置每个XML元素出现和结束的处理函数。这里设置start为元素开始处理函数，end元素结束处理函数。*/
	XML_SetUserData(parser, this);
	XML_SetElementHandler(parser, tag_start, tag_end);
	XML_SetCharacterDataHandler(parser, tag_data);

	/* 调用库函数XML_Parse来分析缓冲区Buff里的XML数据。*/
	if (!XML_Parse(parser, str, len, 1)) {
		char errmsg[1024];
		snprintf(errmsg, 1023, "Parse error at line %ld: %s",
				XML_GetCurrentLineNumber(parser),
				XML_ErrorString(XML_GetErrorCode(parser)));
		throw((const char*)errmsg);
	}

	/* 释放XML解析器 */
	XML_ParserFree(parser);
	init(QAO_CDAT_WX, 0, 0);
}


cdat_wx::~cdat_wx() {
	if (data) {
		delete[] data;
	}
}


void cdat_wx::tag_start(void *data, const char *tag, const char **attr) {
	cdat_wx *pwx = (cdat_wx*)data;
	if (!strcmp(tag, "ToUserName")) {
		pwx->tag_type = CFG_TAG_TOUSERNAME;
	} else if (!strcmp(tag, "FromUserName")) {
		pwx->tag_type = CFG_TAG_FROMUSERNAME;
	} else if (!strcmp(tag, "CreateTime")) {
		pwx->tag_type = CFG_TAG_CREATETIME;
	} else if (!strcmp(tag, "MsgType")) {
		pwx->tag_type = CFG_TAG_MSGTYPE;
	} else if (!strcmp(tag, "Content")) {
		pwx->tag_type = CFG_TAG_CONTENT;
	} else if (!strcmp(tag, "MsgId")) {
		pwx->tag_type = CFG_TAG_MSGID;
	} else {
		pwx->tag_type = CFG_TAG_DEFAULT;
	}
} 


/* 下面定义一个XML元素结束调用的函数。*/
void cdat_wx::tag_end(void *data, const char *tag) {
}


void cdat_wx::tag_data(void *data, const char *s, int len) {
	cdat_wx *pwx = (cdat_wx*)data;
	
	/* 复制数据 */
	char *buf = new char[len+1];
	memcpy(buf, s, len);
	buf[len] = '\0';

	switch (pwx->tag_type) {
		case CFG_TAG_TOUSERNAME:
			pwx->hash = company_hash(buf);
			break;
			
		case CFG_TAG_FROMUSERNAME:
			pwx->user_hash = company_hash(buf);
			break;
			
		case CFG_TAG_CREATETIME:
			break;
			
		case CFG_TAG_MSGTYPE:
			if (!strcmp(buf, "text")) {
				pwx->type = CFG_WX_TYPE_TEXT;
			} else if (!strcmp(buf, "image")) {
				pwx->type = CFG_WX_TYPE_IMAGE;
			} else if (!strcmp(buf, "location")) {
				pwx->type = CFG_WX_TYPE_LOCATION;
			} else if (!strcmp(buf, "link")) {
				pwx->type = CFG_WX_TYPE_LINK;
			} else if (!strcmp(buf, "event")) {
				pwx->type = CFG_WX_TYPE_EVENT;
			} else {
				pwx->type = CFG_WX_TYPE_UNKNOW;
			}
			break;
			
		case CFG_TAG_CONTENT:
			pwx->data = buf;
			pwx->datalen = len;
			buf = NULL;
			break;
			
		case CFG_TAG_MSGID:
			pwx->msgid = strtoul(buf, NULL, 10);
			break;
			
		default:
			break;
	}
	
	/* 释放缓冲区 */
	if (buf) {
		delete[] buf;
	}
}


char *cdat_wx::serialization(size_t &len) {
	Json::Value serial;
	Json::FastWriter writer;
	
	serial["hash"] = (Json::UInt64)hash;
	serial["user_hash"] = (Json::UInt64)user_hash;
	serial["msgid"] = (Json::UInt64)msgid;
	serial["type"] = type;
	serial["datalen"] = (Json::UInt)datalen;
	serial["data"] = data;
#ifdef CFG_QAO_TRACE	
	serial["trace"] = serial_trace();
#endif
		
	string json_str = writer.write(serial);
	size_t json_len = json_str.length() + 1;
	size_t buf_len = json_len + sizeof(serial_data);
	
	char *ret = new char[buf_len];
	serial_data *pserial = (serial_data*)ret;
	serial_header(pserial, buf_len, json_len);
	memcpy(pserial->data, json_str.c_str(), json_len);
	len = buf_len;
	return ret;
}


void cdat_wx::dump(void) {
}
