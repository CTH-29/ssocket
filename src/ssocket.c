#include "ssocket.h"
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <errno.h>
#include <time.h>


#define INVALID_FD -1
#define CHECK_NULL(p, ret) \
    do                     \
    {                      \
        if (p == NULL)     \
            return ret;    \
    } while (0)

#define CHECK_SOCKET(sock, ret) \
    do                          \
    {                           \
        if (sock < 3)           \
            return ret;         \
    } while (0)

#ifdef SSOCKET_DEBUG
#define _ssocket_debug(...)  \
    do                       \
    {                        \
        printf(__VA_ARGS__); \
        printf("\n");        \
    } while (0)
#else
#define _ssocket_debug(...) ((void)0)
#endif

int _socket_wait(int socket_fd, int dir, int timeout_ms);
bool _check_addr(const char *addr);
void _set_tcp_opts(int socket_fd);

// #define _socket_write_wait(fd, tm) _socket_wait(fd, 2, tm)
// #define _socket_read_wait(fd, tm) _socket_wait(fd, 1, tm)

/**
 * _socket_wait(): wait the socket ready to read or write.
 * @dir: 1 for read, 2 for write, 3 for both.
 * @timeout_ms: 1000ms = 1s
 * Return: 0 for ready, 1 for invilad params, 2 for timeout and 3 for connect errors.
 */
int _socket_wait(int socket_fd, int dir, int timeout_ms)
{
    int ret;
    fd_set rd_fds;
    fd_set wr_fds;
    struct timeval timeout;

    CHECK_SOCKET(socket_fd, -1);

    FD_ZERO(&rd_fds);
    FD_ZERO(&wr_fds);

    if (dir & 0x01)
        FD_SET(socket_fd, &rd_fds);
    if (dir & 0x02)
        FD_SET(socket_fd, &wr_fds);

    timeout.tv_sec = 0;
    timeout.tv_usec = timeout_ms * 1000;

    ret = select(socket_fd + 1, &rd_fds, &wr_fds, NULL, &timeout);

    if (ret <= -1)
        return 3;
    else if (ret == 0)
        return 2;
    else
        return 0;
}

bool _check_hostname(const char *hostname)
{
    int i, len = strlen(hostname);
    for (i = 0; i < len; i++)
        if (!((hostname[i] >= '0' && hostname[i] <= '9') || hostname[i] == '.'))
            return false;
    return true;
}

void _set_tcp_opts(int socket_fd)
{
    int tcp_nodelay = 1;
    int tcp_keepalive = 1;
    int tcp_keepcnt = 1;
    int tcp_keepidle = 45;
    int tcp_keepintvl = 30;
    struct timeval timeout = {3, 0};

    setsockopt(socket_fd, SOL_TCP, TCP_NODELAY, &tcp_nodelay, sizeof(tcp_nodelay));
    setsockopt(socket_fd, SOL_SOCKET, SO_KEEPALIVE, (const void *)&tcp_keepalive, sizeof(tcp_keepalive));
    setsockopt(socket_fd, SOL_TCP, TCP_KEEPCNT, &tcp_keepcnt, sizeof(tcp_keepcnt));
    setsockopt(socket_fd, SOL_TCP, TCP_KEEPIDLE, &tcp_keepidle, sizeof(tcp_keepidle));
    setsockopt(socket_fd, SOL_TCP, TCP_KEEPINTVL, &tcp_keepintvl, sizeof(tcp_keepintvl));
    setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));
    setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));
    /* make socket_fd close on exec */
    fcntl(socket_fd, F_SETFD, FD_CLOEXEC);
}

ssocket_t *ssocket_create(int timeout_conn, int timeout_recv, int timeout_send)
{
    ssocket_t *sso = (ssocket_t *)malloc(sizeof(ssocket_t));
    if (sso == NULL)
        return NULL;
    sso->fd = INVALID_FD;
    sso->timeout_conn = timeout_conn > 0 ? timeout_conn : 0;
    sso->timeout_recv = timeout_recv > 0 ? timeout_recv : 0;
    sso->timeout_send = timeout_send > 0 ? timeout_send : 0;
    return sso;
}

void ssocket_destory(ssocket_t *sso)
{
    if (sso)
    {
        if (sso->fd != INVALID_FD)
            ssocket_disconnect(sso);
        if (sso->protocol)
            free(sso->protocol);
        if (sso->ip)
            free(sso->ip);
        free(sso);
    }
}

int _socket_connect(int domain, int type, int protocol, struct sockaddr *server, int timeout)
{
    int socket_fd, flags, ret;
    int error = 0;
    socklen_t length = sizeof(error);
    struct sockaddr_in *server_ptr;

    socket_fd = socket(domain, type, protocol);
    if (socket_fd == INVALID_FD)
        return -1;

    /* set socket to non-blocking mode */
    flags = fcntl(socket_fd, F_GETFL, 0);
    fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);

    server_ptr = (struct sockaddr_in *)server;
    char *addr_str = inet_ntoa(server_ptr->sin_addr);
    _ssocket_debug("ssocket connect %s %s:%d", server_ptr->sin_family == AF_INET ? "ipv4" : "not ipv4", addr_str, ntohs(server_ptr->sin_port));

    if (connect(socket_fd, server, sizeof(struct sockaddr)) == 0)
        return socket_fd;
    else if (errno != EINPROGRESS)
        goto close_and_exit;

    /*
     * use select to check write event, if the socket is writable and no errors,
     * then connect is complete successfully!
     */
    if (_socket_wait(socket_fd, 2, timeout) != 0)
    {
        _ssocket_debug("ssocket connect timeout");
        goto close_and_exit;
    }

    if (getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &error, &length) < 0)
        goto close_and_exit;

    if (error != 0)
        goto close_and_exit;

    fcntl(socket_fd, F_SETFL, flags);
    _set_tcp_opts(fd);
    return socket_fd;

close_and_exit:
    close(socket_fd);
    return -1;
}

bool ssocket_connect_hostname(ssocket_t *sso, const char *hostname, const char *port)
{
    unsigned short port_num;
    struct hostent *hosts;
    struct sockaddr_in host;
    port_num = atoi(port);

    CHECK_NULL(sso, false);
    CHECK_NULL(hostname, false);
    CHECK_NULL(port, false);

    if (_check_hostname(hostname)) /* only consist of number and dot */
        return ssocket_connect_ip(sso, hostname, port_num);
    hosts = gethostbyname(hostname);
    if (hosts == NULL)
    {
        _ssocket_debug("ssocket get host failed for %s", hostname);
        return false;
    }

    // printf("hosts->h_name = %s\n", hosts->h_name);
    // for (char **ptr = hosts->h_aliases; *ptr; ptr++)
    //     printf("hosts->h_aliases = %s\n", *ptr);
    // printf("hosts->h_addrtype = %d\n", hosts->h_addrtype);
    // printf("hosts->h_length = %d\n", hosts->h_length);
    // for (char **ptr = hosts->h_addr_list; *ptr; ptr++)
    //     printf("hosts->h_addr_list = %s\n", inet_ntoa(*((struct in_addr* )(*ptr))));


    if(hosts->h_addrtype != AF_INET || hosts->h_length != 4)
        _ssocket_debug("ssocket host not have ipv4 addr");

    for (char **ptr = hosts->h_addr_list; *ptr; ptr++)
    {
        host.sin_family = AF_INET;
        host.sin_addr.s_addr = (*(in_addr_t *)(*ptr));
        host.sin_port = htons(port_num);
        sso->fd = _socket_connect(AF_INET, SOCK_STREAM, IPPROTO_TCP, (struct sockaddr *)&host, sso->timeout_conn);
        if (sso->fd != INVALID_FD)
            goto OK;
    }

    _ssocket_debug("ssocket connect failed");
    return false;

OK:
    _ssocket_debug("ssocket connect success");
    sso->protocol = strdup("tcp");
    sso->ip = strdup(inet_ntoa(host.sin_addr));
    sso->port = port_num;
    return true;
}

bool ssocket_connect_ip(ssocket_t *sso, const char *ip, unsigned short port)
{
    struct sockaddr_in host;
    CHECK_NULL(sso, false);
    CHECK_NULL(ip, false);

    in_addr_t _addr = inet_addr(ip); /* only consider ipv4 */
    if (_addr == INADDR_NONE)
    {
        _ssocket_debug("ssocket invalid ip %s", ip);
        return false;
    }

    host.sin_family = AF_INET;
    host.sin_addr.s_addr = _addr;
    host.sin_port = htons(port);

    sso->fd = _socket_connect(AF_INET, SOCK_STREAM, IPPROTO_TCP, (struct sockaddr *)&host, sso->timeout_conn);

    if (sso->fd != INVALID_FD)
    {
        _ssocket_debug("ssocket connect success");
        sso->protocol = strdup("tcp");
        sso->ip = strdup(inet_ntoa(host.sin_addr));
        sso->port = port;
        return true;
    }
    else
    {
        _ssocket_debug("ssocket connect failed");
        return false;
    }
}

bool ssocket_disconnect(ssocket_t *sso)
{
    CHECK_NULL(sso, false);
    shutdown(sso->fd, SHUT_RDWR);
    close(sso->fd);
    sso->fd = INVALID_FD;
    return true;
}

bool ssocket_send(ssocket_t *sso, const char *sbuf, int sbuf_len)
{
    int send_offset = 0;
    CHECK_NULL(sso, false);
    CHECK_SOCKET(sso->fd, false);
    CHECK_NULL(sbuf, false);

    while (sbuf_len > 0)
    {
        int n;
        if (_socket_wait(sso->fd, 2, sso->timeout_send) > 0)
            return false;
        n = send(sso->fd, sbuf + send_offset, sbuf_len, MSG_NOSIGNAL);
        if (n < 0)
        {
            if (errno != EAGAIN || errno != EWOULDBLOCK)
                return false;
            n = 0;
        }
        send_offset += n;
        sbuf_len -= n;
    }
    return true;
}
int ssocket_recv(ssocket_t *sso, char *rbuf, int rbuf_len)
{
    int ret;
    CHECK_NULL(sso, 0);
    CHECK_SOCKET(sso->fd, 0);
    CHECK_NULL(rbuf, 0);

    if (_socket_wait(sso->fd, 1, sso->timeout_recv) == 0)
    {
        ret = recv(sso->fd, rbuf, rbuf_len, 0);
        if (ret > 0)
        {
            rbuf[ret] = 0;
            return ret;
        }
        else
            return 0; /* TCP socket has been disconnected. */
    }
    else
    {
        return 0;
    }
}

bool ssocket_recv_ready(ssocket_t *sso, int time_out)
{
    CHECK_NULL(sso, false);
    CHECK_SOCKET(sso->fd, false);
    time_out = time_out > 0 ? time_out : 0;
    return (_socket_wait(sso->fd, 1, time_out) == 0) ? true : false;
}

void ssocket_dump(ssocket_t *sso)
{
    printf("ssocket_t dump:\n");
    printf("\tfd = %d\n", sso->fd);
    printf("\tprotocol = %s\n", sso->protocol);
    printf("\tip = %s\n", sso->ip);
    printf("\tport = %d\n", sso->port);
    printf("\ttimeout = %d,%d,%d\n", sso->timeout_conn, sso->timeout_recv, sso->timeout_send);
}