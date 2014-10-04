#include "util.h"
#include <time.h>   
#include <sys/time.h> 
#include "protocol.h"
#include "anet.h"

int main(int argc, char **argv)
{
	struct timeval startTime,endTime;  
    float Timeuse;  
    int i,j;  
      
    gettimeofday(&startTime,NULL); 

	char *msg, *q, c, type;
	char *host = "127.0.0.1";
	int port = 19008, fd;
	logLevel = LOG_ROW;

	while ((c = getopt(argc, argv, "h:p:dq:")) != -1) {
		switch (c) {
			case 'h':
				host = optarg;
				printf("set host %s\n", host);
				break;
			case 'p':
				port = atoi(optarg);
				printf("set port %d\n", port);
				break;
			case 'd':
				logLevel = LOG_DEBUG;
				printf("set debug mode %d \n", LOG_DEBUG);
				break;
			case 'q':
				q = optarg;
				// printf("press 1 query LOOKUP_TYPE_ADDR\n");
				// printf("press 2 query LOOKUP_TYPE_PORT\n");
				// scanf("%d", &type);
				type = 1;
				printf("set query %s:%d \n", q, type);
				break;
			default:
				printf("error input\n");
                exit(0);
                break;
		}
	}

	char **s = malloc(sizeof(char *) * 5);
	unsigned char *t = malloc(sizeof(unsigned char) * 5);
	*(s+0) = q;
	t[0] = type;
	
	lookup_protocol *req = new_lookup_request(s, t, 1);
	dump_request(req);
	
	char *p = build_request_str(req);
	int nwrite = 0;
	for (i=0; i<1000;i++) {
		printf("begin i:%d\n", i);
		nwrite = 0;
		if ((fd = anetTcpConnect(msg, host, port)) < 0) {
			lookup_log(LOG_ROW, "error: %s\n", msg);
			continue;
		}

		if ((nwrite = anetWrite(fd, p, req->body_len + PROTOCOL_HAEDER_SIZE)) != (req->body_len + PROTOCOL_HAEDER_SIZE)) {
			close(fd);
			continue;
		}

		char buf_header[4];
		if (anetRead(fd, buf_header, PROTOCOL_HAEDER_SIZE) != PROTOCOL_HAEDER_SIZE) {
			close(fd);
			continue;
		}
		lookup_protocol *resp = parse_protocol_header(buf_header);
		char buf_body[resp->body_len];
		if (anetRead(fd, buf_body, resp->body_len) != resp->body_len) {
			close(fd);
			continue;
		}
		parse_protocol_body(buf_body, resp);

		dump_request(resp);
		close(fd);
	}

	
	gettimeofday(&endTime,NULL);  
      
    Timeuse = 1000000*(endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec);  
      
    Timeuse /= 1000000;  

    printf("Timeuse = %f\n",Timeuse);  	

	return 0;
}