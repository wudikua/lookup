#ifndef EVENT_H
#define EVENT_H

#include <sys/epoll.h>
#include "util.h"
#include "protocol.h"

struct event;

typedef int (*noti_chain_callback) (struct event *, int);

typedef struct event {
	noti_chain_callback cb;
	lookup_protocol *data;
	char *body_data;
	int wr_pos;
	int body_data_size;
	int body_data_wr_pos;
} event;

typedef struct event_pool {
    int maxfd;
    int epfd;
    struct epoll_event epollEv[0];
    event events[0];
} event_pool;

event_pool *create_event(int size);
int add_event(event_pool *pool, int fd, int mask, noti_chain_callback cb);
int del_event(event_pool *pool, int fd);
int set_event(event_pool *pool, int fd, int mask);
int handle_event(event_pool *pool, int to);

#endif