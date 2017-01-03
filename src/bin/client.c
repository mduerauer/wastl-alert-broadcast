#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<sys/socket.h>

#include "wab.h"
#include "client.h"

#define	BUFLEN 512

static  int     udpPort = 8888;
static 	char*	server = "192.168.122.20";

int main(int argc, char* argv[]) {
	int rv = 0;
	struct sockaddr_in si_other;
	int s, i, slen=sizeof(si_other);
	char buf[BUFLEN];
	char message[BUFLEN];

	if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		return 1;
	}

	memset((char *) &si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(udpPort);

	if (inet_aton(server, &si_other.sin_addr) == 0) 
	{
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}

	while(1) {
		memset(buf,'\0', BUFLEN);
		if (recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen) == -1) {
			return 1;
		}

		printf("%s", buf);
	}

	return rv;
}
