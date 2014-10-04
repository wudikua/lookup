#include "util.h"
#include <getopt.h>
#include "server.h"
#include "protocol.h"

int main(int argc, char **argv)
{
	char *host = "0.0.0.0";
	int c, port = 19008;
	logLevel = LOG_ROW;
	while ((c = getopt(argc, argv, "c::h:p:d")) != -1) {
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
			default:
				printf("error input\n");
                exit(0);
                break;
		}
	}
	
	initLookupTable();
	addLookupEntry("test", "127.0.0.1", LOOKUP_TYPE_ADDR);
	addLookupEntry("test", "3389", LOOKUP_TYPE_PORT);

	initServer(host, port);
	
	return 0;
}