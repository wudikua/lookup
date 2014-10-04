#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "util.h"
#include <sys/types.h>

#define VERSION(info) (info & 0xc0) >> 7
#define IS_ANSWER(info) ((info & 0x30) >> 6 == 1)
#define SET_ANSWER(info) (info &= 0xff)
#define SET_QUESTION(info) (info &= 0xdf)
#define IS_QUESTION(info) ((info & 0x30) >> 6 == 0)
#define LOOKUP_TYPE_ADDR 1<<0
#define LOOKUP_TYPE_PORT 1<<1
#define PROTOCOL_HAEDER_SIZE 3
/**
 * 协议头
 * 1个字节 版本信息（7,8位） 问题或者回答 （6位） (5-1位补0)
 * 2个字节 BODEY长度
 * 
 * 协议体
 * 1个字节 问题或回答的类型
 * 2个字节 长度
 * 不确定字节 数据 请求的时候代表问题的关键字，响应时候代表回答内容
 */

typedef struct lookup_body {
	unsigned char type;
	ushort len;
	char *data; 	//问题时data代表请求关键字 回答时data代表回答内容
} lookup_body;

typedef struct lookup_protocol {
	unsigned char info;
	ushort body_len;
	lookup_body body[0];
} lookup_protocol;

char* build_request_str(lookup_protocol *pro);
lookup_protocol* parse_request(char *input);
lookup_protocol* new_lookup_request(char **ques, unsigned char *ques_type, int n);
void dump_request(lookup_protocol *pro);

#endif