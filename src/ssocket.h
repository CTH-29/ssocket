#ifndef __SSOCKET_H__
#define __SSOCKET_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/socket.h>

#define SSOCKET_DEBUG

typedef struct _ssocket_t
{
    int fd;
    char *rbuf;
    char *sbuf;
    int rbuf_len;
    int sbuf_len;
    int rbuf_size;
    int sbuf_size;
    char *protocol;
    char *hostname;
    unsigned short port;
    int timeout_conn;
    int timeout_recv;
    int timeout_send;
} ssocket_t;

/* simplified socket interface */

ssocket_t *ssocket_create(int rbuf_size, int sbuf_size, int timeout_conn, int timeout_recv, int timeout_send);
void ssocket_destory(ssocket_t *sso);

bool ssocket_set_url(ssocket_t *sso, const char *url);
bool ssocket_set_addr(ssocket_t *sso, const char *protocol, const char *hostname, unsigned short port);

bool ssocket_connect(ssocket_t *sso);
bool ssocket_disconnect(ssocket_t *sso);

bool ssocket_send_clear(ssocket_t *sso);
bool ssocket_recv_clear(ssocket_t *sso);
bool ssocket_send(ssocket_t *sso);
bool ssocket_recv(ssocket_t *sso);

bool ssocket_recv_ready(ssocket_t *sso);
bool ssocket_printf(ssocket_t *sso, const char *fmt, ...);

void ssocket_dump(ssocket_t *sso);

#endif
