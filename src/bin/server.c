#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <netdb.h>
#include <pthread.h>
#include <regex.h>

#include "wab.h"
#include "server.h"

static  int broadcastSock = -1;
static  int udpPort = 8888;
static  int loop = 1;
static  int on;
static  int keepalive_interval = 10; // 10 seconds

pthread_mutex_t bc_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char* argv[]) {
    int rv = 0;
    int ka_rc, read_rc;
    pthread_t ka_thread, read_thread;

    bc_init();

    if((ka_rc=pthread_create(&ka_thread, NULL, &keepalive, NULL))) {
        fprintf(stderr, "Thread creation failed: %d\n", ka_rc);
    }

    if((read_rc=pthread_create(&read_thread, NULL, &read_input, NULL))) {
        fprintf(stderr, "Thread creation failed: %d\n", read_rc);
    }

    printf("Starting!\n");

    pthread_join(ka_thread, NULL);
    pthread_join(read_thread, NULL);
    return rv;
}

void *read_input() {
    char * line;
    size_t size = BUFLEN;
    line = malloc(size);
    while(getline(&line, &size, stdin) != -1) {
        process_input(line);
    }
}

void process_input(const char* input) {
    char * regexString = POCSAG_REGEXP;
    size_t maxGroups = 4;
    regex_t regexCompiled;
    regmatch_t groupArray[maxGroups];
    wab_alert_msg msg;

    char buf[BUFLEN];

    if (regcomp(&regexCompiled, regexString, REG_EXTENDED))
    {
        fprintf(stderr, "Could not compile regular expression.\n");
        return;
    };

    if (regexec(&regexCompiled, input, maxGroups, groupArray, 0)) {
        fprintf(stderr, "Invalid input: %s\n", input);
        return;
    }

    msg.ts = (int) time(NULL);
    msg.msgtype = WAB_MSG_ALERT;

    // RIC
    strcpy(buf, input);
    buf[groupArray[1].rm_eo] = 0;
    msg.ric = atoi(buf + groupArray[1].rm_so);

    // SUBRIC
    strcpy(buf, input);
    buf[groupArray[2].rm_eo] = 0;
    msg.subric = atoi(buf + groupArray[2].rm_so);

    // TEXT
    strcpy(buf, input);
    buf[groupArray[3].rm_eo] = 0;
    sprintf(msg.text, "%s", buf + groupArray[3].rm_so);

    dump_msg(stdout, &msg);

    broadcast(&msg);
}

void *keepalive() {
    int n = 0;

    while(loop) {
        pthread_mutex_lock(&bc_mutex);
        n++;
        bc_keepalive();
        pthread_mutex_unlock(&bc_mutex);
        sleep(keepalive_interval);
    }

}

static void bc_init() {
    broadcastSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    on = 1;
    if(setsockopt(broadcastSock, SOL_SOCKET, SO_BROADCAST, (int *)&on, sizeof(on)) < 0) {
        perror("setsockopt");
        exit(EX_UNAVAILABLE);
    }
    shutdown(broadcastSock, SHUT_RD);
}


static void bc_keepalive() {
    wab_alert_msg msg;

    msg.ts = (int) time(NULL);
    msg.msgtype = WAB_MSG_KEEPALIVE;
    sprintf(msg.text, "%s", "Keepalive");

    broadcast(&msg);
}

static void bc_testalert() {
    wab_alert_msg msg;

    msg.ts = (int) time(NULL);
    msg.msgtype = WAB_MSG_ALERT;
    msg.ric = 123;
    msg.subric = SUBRIC_A;
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

static void dump_msg(FILE *stream, const struct wab_alert_msg *msg) {
    fprintf(stream, "MSG [ TYPE: %s, TS: %d, RIC: %d, SUBRIC: %d, TEXT: \"%s\"]\n", msg->msgtype==WAB_MSG_ALERT ? "Alert":"Keepalive", msg->ts, msg->ric, msg->subric, msg->text);
}
