#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

#include "wab.h"
#include "server.h"

static    int    broadcastSock = -1;
static    int    udpPort = 8888;
static    int    loop = 1;

int main(int argc, char* argv[]) {
    int rv = 0;
    int on;
    int n = 0;

    broadcastSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    on = 1;
    if(setsockopt(broadcastSock, SOL_SOCKET, SO_BROADCAST, (int *)&on, sizeof(on)) < 0) {
        perror("setsockopt");
        return EX_UNAVAILABLE;
    }
    shutdown(broadcastSock, SHUT_RD);

    printf("Starting!\n");
    while(loop) {
        n++;
        bc_keepalive();
        printf("Loop %i\n", n);

        if((n % 5) == 0) {
            printf("Test-Alert!\n");
            bc_testalert();
        }

        sleep(1);
    }
    return rv;
}

static void bc_keepalive() {
    wab_alert_msg msg;

    msg.msgtype = WAB_MSG_KEEPALIVE;
    sprintf(msg.text, "%s", "Keepalive");

    broadcast(&msg);
}

static void bc_testalert() {
    wab_alert_msg msg;

    msg.msgtype = WAB_MSG_ALERT;
    sprintf(msg.text, "%s", "Test-Alarm");

    broadcast(&msg);
}


static void broadcast(const struct wab_alert_msg *msg) {
    struct sockaddr_in s;
    int on;

    if(broadcastSock < 1)
        return;

    memset(&s, '\0', sizeof(struct sockaddr_in));
    s.sin_family = AF_INET;
    s.sin_port = (in_port_t)htons(udpPort);
    s.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    if(sendto(broadcastSock, msg, sizeof(*msg), 0, (struct sockaddr *)&s, sizeof(struct sockaddr_in)) < 0)
        perror("sendto");

}
