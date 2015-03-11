#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <log.h>
#include <sys/time.h>
#include <json/json.h>
#include <json/value.h>
#include <qao/qao_base.h>
#include <qao/wx_xml.h>
#include <hash_alg.h>
#include <expat.h>

using namespace std;

unsigned int wx_xml::seqno = 0;


wx_xml::wx_xml(const char *str, size_t len) {
	/* 创建一个XML分析器。*/
	XML_Parser parser = XML_ParserCreate(NULL);
	if (!parser) {
		throw "Couldn't allocate memory for parser";
	}

	/* 下面设置每个XML元素出现和结束的处理函数。这里设置start为元素开始处理函数，end元素结束处理函数。*/
	XML_SetUserData(parser, this);
	XML_SetElementHandler(parser, tag_start, tag_end);
	XML_SetCharacterDataHandler(parser, tag_data);

	/* 调用库函数XML_Parse来分析缓冲区Buff里的XML数据。*/
	if (!XML_Parse(parser, str, len, 1)) {
		throw "parser xml failed";
	}
	
	XML_ParserFree(parser);
	type = CFG_WX_TYPE_UNKNOW;
	data = NULL;
}


void wx_xml::tag_start(void *data, const char *tag, const char **attr) {
	wx_xml *pwx = (wx_xml*)data;
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
void wx_xml::tag_end(void *data, const char *tag) {
}


void wx_xml::tag_data(void *data, const char *s, int len) {
	wx_xml *pwx = (wx_xml*)data;
	
	/* 复制数据 */
	char *buf = new char[len+1];
	memcpy(buf, s, len);
	buf[len] = '\0';
	
	switch (pwx->tag_type) {
		case CFG_TAG_TOUSERNAME:
			pwx->hash = company_hash(buf);
			delete buf;
			break;
			
		case CFG_TAG_FROMUSERNAME:
			pwx->user_hash = company_hash(buf);
			delete buf;
			break;
			
		case CFG_TAG_CREATETIME:
			delete buf;
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
			break;
			
		case CFG_TAG_MSGID:
			pwx->msgid = strtoul(buf, NULL, 10);
			delete buf;
			break;
			
		default:
			delete buf;
			break;
	}
}


void *wx_xml::serialization(size_t &len, unsigned long token) {
	Json::Value serial;
	Json::FastWriter writer;
	
	serial["hash"] = (Json::UInt64)hash;
	serial["user_hash"] = (Json::UInt64)user_hash;
	serial["msgid"] = (Json::UInt64)msgid;
	serial["type"] = type;
	serial["datalen"] = (Json::UInt)datalen;
//	serial["data"] = datalen;
		
	string json_str = writer.write(serial);
	size_t json_len = json_str.length() + 1;
	size_t buf_len = json_len + sizeof(serial_data);
	
	void *ret = new unsigned char[buf_len];
	serial_data *pserial = (serial_data*)ret;
	pserial->token = token;
	pserial->length = buf_len;
	pserial->type = QAO_CLB_CTL_REQ;
	pserial->version = (qao_version & 0x3f) | ((qao_qos & 0x3) << 6);
	pserial->datalen = json_len;
	memcpy(pserial->data, json_str.c_str(), json_len);
	len = buf_len;
	return ret;
}


void *wx_xml::serialization(size_t &len) {
	struct timeval tv;
	
	gettimeofday(&tv, NULL);
	unsigned long token = (tv.tv_sec << 8) | QAO_WX_XML;
	token <<= 32;
	token |= __sync_fetch_and_add(&wx_xml::seqno, 1);
	return serialization(len, token);
}


void *wx_xml::serial_xml(size_t &len) {
	return NULL;
}
