#include "event.h"
#include "zmalloc.h"

event_pool * create_event(int size)
{
    event_pool *pool =
        zmalloc(sizeof(event_pool) + 
        	sizeof(event) * size + 
        	sizeof(struct epoll_event) * size);
    int epfd = epoll_create(size);
    if (epfd == 0)
        printf("epoll create failed\n");
    pool->maxfd = size;
    pool->epfd = epfd;
    memset(pool->epollEv, 0,
           sizeof(struct epoll_event) * size);
    memset(pool->events, 0,
           sizeof(event) * size);
    return pool;
}

int add_event(event_pool *pool, int fd, int mask, noti_chain_callback cb)
{
    struct epoll_event ev;
    int ret = 0;
    int epfd = pool->epfd;
    ev.data.fd = fd;
    if (ev.data.fd < 0)
        return -1;
	ev.events = mask;
    pool->events[fd].cb = cb;
    pool->events[fd].data = NULL;
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    if (ret < 0) {
        printf("fd is %d\n", fd);
        perror("epoll_ctl");
    }
    return ret;
}

int set_event(event_pool *pool, int fd, int mask)
{
    struct epoll_event ev;
    int ret = 0;
    int epfd = pool->epfd;
    ev.data.fd = fd;
    if (ev.data.fd < 0)
        return -1;
    ev.events = mask;
    ret = epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
    if (ret < 0) {
        printf("fd is %d\n", fd);
        perror("epoll_ctl");
    }
    return ret;
}

int del_event(event_pool *pool, int fd)
{
    struct epoll_event ev;
    int ret = 0;
    ev.data.fd = fd;
    // memset(&pool->events[fd], 0, sizeof(event));
    ret = epoll_ctl(pool->epfd, EPOLL_CTL_DEL, fd, &ev);
    return ret;
}

int handle_event(event_pool *pool, int to)
{
    int num = 0;
    if (to == 0)
        to = -1;
    else
        to = to * 100;
    while (1) {
        num = epoll_wait(pool->epfd, pool->epollEv, pool->maxfd, to);
        if (num >= 0)
            break;
        if (num < 0 && errno == EINTR)
            continue;
    }
    return num;
}