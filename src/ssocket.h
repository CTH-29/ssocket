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
    char *protocol;
    char *hostname;
    unsigned short port;
    int timeout_conn;
    int timeout_recv;
    int timeout_send;
} ssocket_t;

/* simplified socket interface */

ssocket_t *ssocket_create(int timeout_conn, int timeout_recv, int timeout_send);
void ssocket_destory(ssocket_t *sso);

bool ssocket_set_url(ssocket_t *sso, const char *url);
bool ssocket_set_addr(ssocket_t *sso, const char *protocol, const char *hostname, const char *port);

bool ssocket_connect(ssocket_t *sso);
bool ssocket_disconnect(ssocket_t *sso);

bool ssocket_recv_ready(ssocket_t *sso, int time_out);

bool ssocket_send(ssocket_t *sso, const char *sbuf, int sbuf_len);
int ssocket_recv(ssocket_t *sso, char *rbuf, int rbuf_len);

#define ssocket_send_str(sso,str) ssocket_send(sso, str, strlen(str))

void ssocket_dump(ssocket_t *sso);

#endif
