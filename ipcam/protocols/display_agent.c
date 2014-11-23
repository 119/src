#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <time.h>
#include <event2/event.h>

#include "common.h"
#include "protocol.h"
#include "buffer.h"
#include "display_agent.h"

static void on_break_event(int fd, short what, void *arg)
{

}


static void on_buffer_read(int fd, short what, void *arg)
{
    int len;
    struct buffer_item *item;
    display_agent_t *na = (display_agent_t *)arg;
    struct buffer_ctx *buf_src = na->buf_src;

    while (NULL != (item = buffer_pop(buf_src))) {
        len = protocol_write(na->pc, item->data, item->len);
        if (len == -1) {
            printf("protocol_write failed!\n");
        }
//    printf("%s:%d protocol_write len=%d\n", __func__, __LINE__, len);
//        buffer_item_free(item);
    }
}
static void *display_agent_loop(void *arg)
{
    struct display_agent *na = (struct display_agent *)arg;
    if (!na)
        return NULL;
    event_base_loop(na->ev_base, 0);
    return NULL;
}


struct display_agent *display_agent_create(struct buffer_ctx *buf_src, struct buffer_ctx *buf_snk)
{
    pthread_t tid;
    display_agent_t *na = NULL;
    int fds[2];

    na = (display_agent_t *)calloc(1, sizeof(display_agent_t));
    if (!na) {
        return NULL;
    }
//    na->pc = protocol_new("udp://127.0.0.1:2333");
    na->pc = protocol_new("sdl://player");
    if (!na->pc) {
        return NULL;
    }
    if (-1 == protocol_open(na->pc)) {
        return NULL;
    }
    na->ev_base = event_base_new();
    if (!na->ev_base) {
        return NULL;
    }

    na->buf_src = buf_src;
    na->buf_snk = buf_snk;
    na->ev_read = event_new(na->ev_base, buf_src->pipe_rfd, EV_READ|EV_PERSIST, on_buffer_read, na);
    if (!na->ev_read) {
        return NULL;
    }
    if (-1 == event_add(na->ev_read, NULL)) {
        return NULL;
    }
    if (pipe(fds)) {
        printf("create pipe failed: %s\n", strerror(errno));
        return NULL;
    }
    na->pipe_rfd = fds[0];
    na->pipe_wfd = fds[1];

    na->ev_close = event_new(na->ev_base, na->pipe_rfd, EV_READ|EV_PERSIST, on_break_event, na->pc);
    if (!na->ev_close) {
        return NULL;
    }
    if (-1 == event_add(na->ev_close, NULL)) {
        return NULL;
    }

    if (0 != pthread_create(&tid, NULL, display_agent_loop, na)) {
        printf("pthread_create falied: %s\n", strerror(errno));
        free(na);
        return NULL;
    }

    return na;
}