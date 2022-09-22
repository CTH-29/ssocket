#ifndef __SSOCKET_H__
#define __SSOCKET_H__

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define SSOCKET_DEBUG

typedef struct _ssocket_t
{
    int fd;
    char *protocol;
    char *ip;
    unsigned short port;
    int timeout_conn;
    int timeout_recv;
    int timeout_send;
} ssocket_t;

/* simplified socket interface */

ssocket_t *ssocket_create(int timeout_conn, int timeout_recv, int timeout_send);
void ssocket_destory(ssocket_t *sso);

bool ssocket_connect_hostname(ssocket_t *sso,const char *hostname,const char *port);
bool ssocket_connect_ip(ssocket_t *sso,const char *ip,unsigned short port);

bool ssocket_disconnect(ssocket_t *sso);

bool ssocket_recv_ready(ssocket_t *sso, int time_out);

bool ssocket_send(ssocket_t *sso, const char *sbuf, int sbuf_len);
int ssocket_recv(ssocket_t *sso, char *rbuf, int rbuf_len);

#define ssocket_send_str(sso,str) ssocket_send(sso, str, strlen(str))

void ssocket_dump(ssocket_t *sso);

#endif
