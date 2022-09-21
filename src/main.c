#include "ssocket.h"

char rbuf[2048];
char sbuf[2048];

int main(int argc, char *argv[])
{
    ssocket_t *sso = ssocket_create(2000, 1000, 1000);
    ssocket_set_addr(sso, "tcp", "172.28.83.246", 8848);
    if (!ssocket_connect(sso))
        return 0;
    while (1)
    {
        if (ssocket_recv_ready(sso))
        {
            if (ssocket_recv(sso, rbuf, 2048))
            {
                printf("socket recv:%s\n", rbuf);
                ssocket_send_str(sso, "recv:");
                ssocket_send_str(sso, rbuf);

                if (strcmp(rbuf, "close") == 0)
                {
                    printf("socket close\n");
                    break;
                }
            }
            else
            {
                break;
            }
        }
    }
    ssocket_disconnect(sso);
    return 0;
}
