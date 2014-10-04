#include "protocol.h"
#include "anet.h"

char* build_request_str(lookup_protocol *pro)
{
	int len = sizeof(char) + sizeof(ushort) + pro->body_len;
	lookup_log(LOG_DEBUG, "build len:%d\n", len);
	char *ret = (char *)zmalloc(len);
	memset(ret, 0, len);
	char *p = ret;
	*p++ = pro->info;
	*p++ = pro->body_len & (0xff << 8);
	lookup_log(LOG_DEBUG, "body_len1 :%d\n", pro->body_len & (0xff << 8));
	*p++ = pro->body_len & 0xff;
	lookup_log(LOG_DEBUG, "body_len2 :%d\n", pro->body_len & 0xff);
	lookup_body *b = pro->body;
	int nwrite = 0;
	while (nwrite < pro->body_len) {
		*p++ = (*b).type;
		*p++ = (*b).len & 0xf0;
		*p++ = (*b).len & 0x0f;
		char *pData = (*b).data;
		int i = 0;
		while (i < (*b).len) {
			*p++ = *(pData + i);
			i++;
		}
		b++;
		nwrite += sizeof(*b->data) + 3;
	}
	return ret;
}

int check_protocol_header(char *str)
{
	if (VERSION(str[0]) == 1) {
		return 0;
	} else {
		return -1;
	}
}

lookup_protocol* parse_protocol_header(char *str) 
{
	lookup_protocol *pro = (lookup_protocol *)zmalloc(sizeof(lookup_protocol) + sizeof(lookup_body) * 8);
	lookup_log(LOG_DEBUG, "alloc pro addr:%d\n", pro);
	char *p = str;
	pro->info = *p++;
	unsigned char c1 = *p++;
	unsigned char c2 = *p++;
	pro->body_len = c1 << 8 | c2;
	return pro;
}

int parse_protocol_body(char *str, lookup_protocol *pro)
{
	char *p = str;
	int nwrite = 0;
	int body_count = 0;
	unsigned char c1;
	unsigned char c2;
	lookup_log(LOG_DEBUG, "parse body:%s\n", str+3);
	while (nwrite < pro->body_len) {
		lookup_body *b = &pro->body[body_count];
		b->type = *p++;
		c1 = *p++;
		c2 = *p++;
		b->len = c1<<8 | c2;
		lookup_log(LOG_DEBUG, "str:%d\n", b->len);
		b->data = (void *)zmalloc(sizeof(char) * b->len);
		int i = 0;
		while (i < b->len) {
			b->data[i++] = *p++;
		}
		nwrite += b->len;
		body_count++;
	}
	return 0;
}

lookup_protocol* parse_request(char *str)
{
	lookup_protocol *pro = (lookup_protocol *)zmalloc(sizeof(lookup_protocol) + sizeof(lookup_body) * 8);
	char *p = str;
	pro->info = *p++;
	lookup_log(LOG_DEBUG, "pro info:%02x\n", pro->info);
	unsigned char c1 = *p++;
	unsigned char c2 = *p++;
	lookup_log(LOG_DEBUG, "pro len c1:%d %02x\n", c1, c2);
	lookup_log(LOG_DEBUG, "pro len c2:%d %02x\n", c2, c2);
	pro->body_len = c1 << 8 | c2;
	int nwrite = 0;
	int body_count = 0;
	lookup_log(LOG_DEBUG, "pro len:%02x\n", pro->body_len);
	while (nwrite < pro->body_len) {
		char *pb = p;
		lookup_body *b = &pro->body[body_count];
		lookup_log(LOG_DEBUG, "0\n");
		b->type = *p++;
		c1 = *p++;
		c2 = *p++;
		b->len = c1 << 8 | c2;
		lookup_log(LOG_DEBUG, "b len:%02x\n", b->len);
		b->data = (void *)zmalloc(sizeof(char) * b->len);
		int i = 0;
		while (i < b->len) {
			b->data[i++] = *p++;
		}
		lookup_log(LOG_DEBUG, "result i:%d\n", i);
		lookup_log(LOG_DEBUG, "b data:%s\n", b->data);
		nwrite += p - pb;
		body_count++;
		lookup_log(LOG_DEBUG, "1\n");
	}
	lookup_log(LOG_DEBUG, "2\n");
	return pro;
}

lookup_protocol* new_lookup_request(char **ques,unsigned char *ques_type, int n)
{
	lookup_protocol *req = malloc(sizeof(lookup_protocol) + sizeof(lookup_body) * 8);
	req->info = 1<<7 | 1<6;
	int i = 0;
	int body_len = 0;
	while (i < n) {
		char *que = *(ques + i);
		lookup_log(LOG_DEBUG, "new lookup %s\n", que);
		lookup_body *body = &req->body[i];
		body->type = ques_type[i];
		body->len = strlen(que) + 1;
		body->data = que;
		req->body[i++] = *body;
		body_len += body->len + 3;
	}
	req->body_len = body_len;
	return req;
}

void dump_request(lookup_protocol *pro) {
	lookup_log(LOG_ROW, "pro->info:%02x\n", pro->info);
	lookup_log(LOG_ROW, "pro->version:%d\n", VERSION(pro->info));
	lookup_log(LOG_ROW, "pro->answer:%d pro->question:%d\n", 
		IS_ANSWER(pro->info), IS_QUESTION(pro->info));
	lookup_log(LOG_ROW, "pro->body_len:%d\n", pro->body_len);
	int tmp = 0;
	int i = 0;
	while (tmp < pro->body_len) {
		lookup_log(LOG_ROW, "pro->body[%d] type: %02x %d\n", i, pro->body[i].type, pro->body[i].type);
		lookup_log(LOG_ROW, "pro->body[%d] len: %d\n", i, pro->body[i].len);
		lookup_log(LOG_ROW, "pro->body[%d] data: %s\n", i, pro->body[i].data);
		tmp += pro->body[i].len + 3;
		i++;
	}
}
