#ifndef SERVER_H
#define SERVER_H

#include "event.h"
#include "dict.h"

#define IO_PACKET_SIZE 4

typedef struct lookupKey
{
	char *name;
	unsigned char type;
} lookupKey;

event_pool *pool;

dict *dt;

int serverWrite(event *ev, int fd);
int serverRead(event *ev, int fd);
int serverAccept(event *v, int fd);
void initServer(char *host, int port);

#endif