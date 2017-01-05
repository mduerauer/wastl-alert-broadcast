#ifndef server_h
#define server_h

#define EX_UNAVAILABLE  1
#define EX_INPUT_UNACCESSIBLE 2

#define RE_CHARACTER_CLASSES    1

#define POCSAG_REGEXP   "^POCSAG1200:\\s+Address:\\s+([0-9]+)\\s+Function:\\s+([0-9]+)\\s+Alpha:\\s+(.*)$"

#define DEFAULT_POLL_INTERVAL   10
#define DEFAULT_KEEPALIVE_INTERVAL 10

void *read_input();
void process_input(const char* input);
void *keepalive();
void display_usage();
static void broadcast(const struct wab_alert_msg *msg);
static void bc_init();
static void bc_keepalive();
static void bc_testalert();
static void dump_msg(FILE * stream, const struct wab_alert_msg *msg);
void wab_log(int priority, const char* format, ... /* arguments */ );

#endif
