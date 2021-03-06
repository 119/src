#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <sys/epoll.h>
#include "libskt_event.h"
#include "libskt_log.h"

#define EPOLL_MAX_NEVENT	4096
#define MAX_SECONDS_IN_MSEC_LONG \
	(((LONG_MAX) - 999) / 1000)

struct epoll_ctx {
    int epfd;
    int nevents;
    struct epoll_event *events;
};

static void *epoll_init()
{
    int fd;
    struct epoll_ctx *ec;
    fd = epoll_create(1);
    if (-1 == fd) {
        skt_log(LOG_ERR, "errno=%d %s\n", errno, strerror(errno));
        return NULL;
    }
    ec = (struct epoll_ctx *)calloc(1, sizeof(struct epoll_ctx));
    if (!ec) {
        skt_log(LOG_ERR, "malloc epoll_ctx failed!\n");
        return NULL;
    }
    ec->epfd = fd;
    ec->nevents = EPOLL_MAX_NEVENT;
    ec->events = (struct epoll_event *)calloc(EPOLL_MAX_NEVENT, sizeof(struct epoll_event));
    if (!ec->events) {
        skt_log(LOG_ERR, "malloc epoll_event failed!\n");
        return NULL;
    }
    return ec;
}

static int epoll_add(struct event_base *eb, struct skt_ev *e)
{
    struct epoll_ctx *ec = eb->base;
    struct epoll_event epev;
    memset(&epev, 0, sizeof(epev));
    if (e->flags & EVENT_READ)
        epev.events |= EPOLLIN;
    if (e->flags & EVENT_WRITE)
        epev.events |= EPOLLOUT;
    if (e->flags & EVENT_ERROR)
        epev.events |= EPOLLERR;
    epev.events |= EPOLLET;
    epev.data.ptr = (void *)e;

    if (-1 == epoll_ctl(ec->epfd, EPOLL_CTL_ADD, e->evfd, &epev)) {
        skt_log(LOG_ERR, "errno=%d %s\n", errno, strerror(errno));
        return -1;
    }
    return 0;
}

static int epoll_del(struct event_base *eb, struct skt_ev *e)
{
    struct epoll_ctx *ec = eb->base;
    if (-1 == epoll_ctl(ec->epfd, EPOLL_CTL_DEL, e->evfd, NULL)) {
        perror("epoll_ctl");
        return -1;
    }
    return 0;
}

static int epoll_dispatch(struct event_base *eb, struct timeval *tv)
{
    struct epoll_ctx *epop = eb->base;
    struct epoll_event *events = epop->events;
    int i, n;
    int timeout = -1;

    if (tv != NULL) {
        if (tv->tv_usec > 1000000 || tv->tv_sec > MAX_SECONDS_IN_MSEC_LONG)
            timeout = -1;
        else
            timeout = (tv->tv_sec * 1000) + ((tv->tv_usec + 999) / 1000);
    } else {
        timeout = -1;
    }
    n = epoll_wait(epop->epfd, events, epop->nevents, timeout); 
    if (-1 == n) {
        skt_log(LOG_ERR, "errno=%d %s\n", errno, strerror(errno));
        return -1;
    }
    if (0 == n) {
        skt_log(LOG_ERR, "epoll_wait timeout\n");
        return 0;
    }
    for (i = 0; i < n; i++) {
        int what = events[i].events;
        struct skt_ev *e = (struct skt_ev *)events[i].data.ptr;

        if (what & (EPOLLHUP|EPOLLERR)) {
        } else {
            if (what & EPOLLIN)
                e->evcb->ev_in((void *)e->evcb->args);
            if (what & EPOLLOUT)
                e->evcb->ev_out((void *)e->evcb->args);
            if (what & EPOLLRDHUP)
                e->evcb->ev_err((void *)e->evcb->args);
        }
    }
    return 0;
}

const struct event_ops epollops = {
	epoll_init,
	epoll_add,
	epoll_del,
	epoll_dispatch,
};
