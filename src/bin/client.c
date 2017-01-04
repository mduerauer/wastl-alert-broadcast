#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<sys/socket.h>

#include "wab.h"
#include "client.h"

static  int     udpPort = 8888;
static 	char*	server = "192.168.122.20";

int main(int argc, char* argv[]) {
    int rv = 0;
    struct sockaddr_in my_addr;
    struct sockaddr_in their_addr;
    socklen_t addr_len;
    int s, i;
    char buf[BUFLEN];
    char message[BUFLEN];
    wab_alert_msg msg;
    int numbytes;


    if ( (s=socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        return 1;
    }

    memset((char *) &my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(udpPort);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(s, (struct sockaddr *)&my_addr,
        sizeof(struct sockaddr)) == -1) {
        perror("bind");
        exit(1);
    }

    addr_len = sizeof(struct sockaddr);
    
    while(1) {
        memset(buf,'\0', BUFLEN);
        if ((numbytes = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &their_addr, &addr_len)) == -1) {
            return 1;
        }

        memcpy(&msg, buf, sizeof(struct wab_alert_msg));
    
        printf("Got packet from %s\n",inet_ntoa(their_addr.sin_addr));
        printf("Packet is %d bytes long\n",numbytes);
        printf("Timestamp is %d\n", msg.ts);
        printf("RIC is %d\n", msg.ric);
        printf("SUBRIC is %d\n", msg.subric);
        printf("Type is %d\n", msg.msgtype);
        printf("Text is %s\n", msg.text);
        printf("-----------------------------------------\n");
    }

    close(s);

    return rv;
}
