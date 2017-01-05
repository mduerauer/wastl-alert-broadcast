#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <netdb.h>
#include <pthread.h>
#include <regex.h>
#include <syslog.h>
#include <getopt.h>

#include "wab.h"
#include "server.h"

static  int sock_broadcast = -1;
static  int port = DEFAULT_PORT;
static  int loop = 1;
static  int on;
static  int keepalive_interval = DEFAULT_KEEPALIVE_INTERVAL; // 10 seconds
static  int debug = 1;

static  char    input_filename[BUFLEN] = "\0";
static  int     poll_interval = DEFAULT_POLL_INTERVAL;

pthread_mutex_t bc_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t syslog_mutex = PTHREAD_MUTEX_INITIALIZER;

struct main_args_t {
    int verbosity;      /* -v / --verbosity option */
    int port;           /* -p / --port option */
    char file[BUFLEN];  /* -f / --file option */ 
    int interval;       /* -i / --interval option */
    int keepalive;      /* -k / --keepalive-interval option */
} main_args;

static const char *options = "vp:f:i:k:h?";

static const struct option long_options[] = {
    { "verbosity",          no_argument,        NULL, 'v' },
    { "help",               no_argument,        NULL, 'h' },
    { "port",               required_argument,  NULL, 'p' },
    { "interval",           required_argument,  NULL, 'i' },
    { "keepalive-interval", required_argument,  NULL, 'k' },
    { "file",               required_argument,  NULL, 'f' }
};

int main(int argc, char* argv[]) {
    int ka_rc, read_rc, opt, long_index;
    pthread_t ka_thread, read_thread;

    // Handle command line arguments
    opt = getopt_long(argc, argv, options, long_options, &long_index);
    while(opt != -1) {

        switch(opt) {
            case 'v':
                main_args.verbosity++;
                break;
            case 'p':
                main_args.port = atoi(optarg);
                break;
            case 'i':
                main_args.interval = atoi(optarg);
                break;
            case 'k':
                main_args.keepalive = atoi(optarg);
                break;
            case 'f':
                strcpy(main_args.file,optarg);
                break;
            case 'h': // fall through this intentionally
            case '?':
                display_usage();
                break;
            default:
                break;

        }

        opt = getopt_long(argc, argv, options, long_options, &long_index);
    }

    // Initialize syslog
    setlogmask (LOG_UPTO (LOG_NOTICE));
    openlog (PACKAGE, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_DAEMON);
    wab_log(LOG_WARNING, "running %s (version %s)\n", PACKAGE, PACKAGE_VERSION);

    // Set input filename
    if(*main_args.file != '\0') {
        wab_log(LOG_DEBUG, "Input file: %s\n", main_args.file);
        strcpy(input_filename, main_args.file);
    }

    // Set verbosity level
    if(main_args.verbosity > 0) {
        wab_log(LOG_DEBUG, "Verbosity level is %d\n", main_args.verbosity);
        debug = 1;
    }

    // Set port
    if(main_args.port != DEFAULT_PORT && main_args.port != 0) {
        wab_log(LOG_INFO, "Using port %d\n", port);
        port = main_args.port;
    }

    // Set poll interval
    if(main_args.interval != DEFAULT_POLL_INTERVAL && main_args.interval != 0) {
        wab_log(LOG_INFO, "Setting poll interval to %ds\n", main_args.interval);
        poll_interval = main_args.interval;
    }

    // Set keepalive interval
    if(main_args.keepalive != DEFAULT_KEEPALIVE_INTERVAL && main_args.keepalive != 0) {
        wab_log(LOG_INFO, "Setting keepalive interval to %ds\n", main_args.keepalive);
        keepalive_interval = main_args.keepalive;
    }


    // Initialize broadcast socket
    bc_init();

    if((ka_rc=pthread_create(&ka_thread, NULL, &keepalive, NULL))) {
        fprintf(stderr, "Thread creation failed: %d\n", ka_rc);
    }

    if((read_rc=pthread_create(&read_thread, NULL, &read_input, NULL))) {
        fprintf(stderr, "Thread creation failed: %d\n", read_rc);
    }

    // Wait for threads to finish
    pthread_join(ka_thread, NULL);
    pthread_join(read_thread, NULL);
   
    // Close syslog
    closelog ();

    return 0;
}

/**
 * void wab_log(int priority, const char* format, ...)
 * Write message to syslog and when debugging is enabled also to stdout
 */
void wab_log(int priority, const char* format, ... /* arguments */ ) {
    va_list arglist;
    va_start( arglist, format );
    if(main_args.verbosity > 0) {
        printf("%d : ", (int) time(NULL));
        vprintf(format, arglist );
    }
    syslog(priority, format, arglist);
    va_end( arglist );
}

/**
 * void display_usage()
 * Print usage infomration and quit
 */
void display_usage() {
    printf("usage: \n ");
    exit(0);
}

/**
 * void read_input()
 * Read file contents an forward every single line to process_input(char* line)
 */
void *read_input() {
    char * line;
    size_t size = BUFLEN;
    FILE * in = stdin;
    int fmode = 0;
    int first_run = 1;
    int poll = 0;
    fpos_t fpos;

    line = malloc(size);

    if(*input_filename != '\0') {
        wab_log(LOG_DEBUG, "Reader thread input filename is %s\n", input_filename);
        if(access(input_filename, R_OK | W_OK) == -1) {
            wab_log(LOG_CRIT, "Can't access file %s. Terminating.\n", input_filename);
            perror("Can't access file");
            exit(EX_INPUT_UNACCESSIBLE);
        }
        
        fmode = 1;
    }

    do {
        if(fmode == 1) {
            wab_log(LOG_DEBUG, "Opening input file for reading.\n");
            in = fopen(input_filename, "r");
            fsetpos(in, &fpos);
        }

        while(getline(&line, &size, in) != -1) {
            process_input(line);
        }

        if(fmode == 1) {
            fgetpos(in, &fpos);
            wab_log(LOG_DEBUG, "Closing input file.\n");
            fclose(in);
            sleep(poll_interval); // TODO: implement filesystem watching mechanism
            poll = 1;
        }

    } while(poll);

    if(*input_filename != '\0') {
        fclose(in);
    }
}

void process_input(const char* input) {
    char * regexString = POCSAG_REGEXP;
    size_t maxGroups = 4;
    regex_t regex;
    regmatch_t groupArray[maxGroups];
    wab_alert_msg msg;

    char buf[BUFLEN];

    if (regcomp(&regex, regexString, REG_EXTENDED))
    {
        fprintf(stderr, "Could not compile regular expression.\n");
        return;
    };

    if (regexec(&regex, input, maxGroups, groupArray, 0)) {
        wab_log(LOG_WARNING, "Invalid input: %s\n", input);
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

    if(debug) {
        dump_msg(stdout, &msg);
    }

    broadcast(&msg);

    regfree(&regex);
}

/**
 * void keepalive()
 * This function broadcasts a keepalive message every few seconds.
 */
void *keepalive() {
    while(loop) {
        pthread_mutex_lock(&bc_mutex);
        bc_keepalive();
        pthread_mutex_unlock(&bc_mutex);
        sleep(keepalive_interval);
    }

}

/**
 * static void bc_init()
 * Here the broadcast datagram socket is initialized.
 */
static void bc_init() {
    sock_broadcast = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    on = 1;
    if(setsockopt(sock_broadcast, SOL_SOCKET, SO_BROADCAST, (int *)&on, sizeof(on)) < 0) {
        perror("setsockopt");
        exit(EX_UNAVAILABLE);
    }
    shutdown(sock_broadcast, SHUT_RD);
}

/**
 * static void bc_keepalive()
 * This functions actualy sends a keepalive broadcast message.
 */
static void bc_keepalive() {
    wab_alert_msg msg;

    msg.ts = (int) time(NULL);
    msg.msgtype = WAB_MSG_KEEPALIVE;
    msg.ric = 0;
    msg.subric = 0;
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

    if(sock_broadcast < 1)
        return;

    memset(&s, '\0', sizeof(struct sockaddr_in));
    s.sin_family = AF_INET;
    s.sin_port = (in_port_t)htons(port);
    s.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    if(sendto(sock_broadcast, msg, sizeof(*msg), 0, (struct sockaddr *)&s, sizeof(struct sockaddr_in)) < 0)
        perror("sendto");
}

static void dump_msg(FILE *stream, const struct wab_alert_msg *msg) {
    fprintf(stream, "MSG [ TYPE: %s, TS: %d, RIC: %d, SUBRIC: %d, TEXT: \"%s\"]\n", msg->msgtype==WAB_MSG_ALERT ? "Alert":"Keepalive", msg->ts, msg->ric, msg->subric, msg->text);
}
