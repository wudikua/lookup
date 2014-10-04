#include <sys/types.h>
#include <sys/event.h>
#include "event.h"

typedef struct event_pool {
    int maxfd;   /* highest file descriptor currently registered */
    int kqfd;	/* kqueue fd*/
    kevent *events; /* Registered events */
} event_pool;

event_pool * create_event(int size)
{
    event_pool *pool =
        malloc(sizeof(event_pool) + sizeof(sizeof(struct kevent)) * size);
    int epfd = epoll_create(size);
    if (epfd == 0)
        printf("epoll create failed\n");
    pool->maxfd = size;
    pool->epfd = epfd;
    return pool;
}

int add_event(event_pool *pool, int fd, int mask)
{
    struct  kevent ke;
    if (mask & AE_READABLE) {
        EV_SET(&ke, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
        if (kevent(pool->kqfd, &ke, 1, NULL, 0, NULL) == -1) return -1;
    }
    if (mask & AE_WRITABLE) {
        EV_SET(&ke, fd, EVFILT_WRITE, EV_ADD, 0, 0, NULL);
        if (kevent(pool->kqfd, &ke, 1, NULL, 0, NULL) == -1) return -1;
    }
    return 0;
}

int del_event(event_pool *pool, int fd, int mask)
{
    struct kevent ke;

    if (mask & AE_READABLE) {
        EV_SET(&ke, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
        kevent(pool->kqfd, &ke, 1, NULL, 0, NULL);
    }
    if (mask & AE_WRITABLE) {
        EV_SET(&ke, fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
        kevent(pool->kqfd, &ke, 1, NULL, 0, NULL);
    }
    return 1;
}

int handle_event(event_pool *pool, int to)
{
    int num = 0;
    if (to == 0)
        to = -1;
    else
        to = to * 100;
    while (1) {
        num = kevent(pool->kqfd, NULL, 0, pool->events, pool->maxfd, &timeout);
        if (num >= 0)
            break;
        if (num < 0 && errno == EINTR)
            continue;
    }
    return num;
}