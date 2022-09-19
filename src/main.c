#include "ssocket.h"

int main(int argc, char *argv[])
{
    ssocket_t *sso = ssocket_create("tcp://172.28.83.246:8848",2048,2048,2000,1000,1000);
    if(!ssocket_connect(sso))
        return 0;
    while(1)
    {
        if(ssocket_recv_ready(sso))
        {
            ssocket_recv_clear(sso);
            ssocket_recv(sso);
            if(sso->rbuf_len == 0)
            {
                printf("socket recv err, disconnect\n");
                break;
            }
            else if(strstr(sso->rbuf,"close"))
            {
                printf("socket recv close and disconnect\n");
                break;
            }
            else
            {
                ssocket_send_clear(sso);
                printf("recv:[%s]",sso->rbuf);
                ssocket_printf(sso,"recv:[%s]",sso->rbuf);
                ssocket_send(sso);
            }
        }
    }
    ssocket_disconnect(sso);
    return 0;
}
