#ifndef server_h
#define server_h

#define EX_UNAVAILABLE  1

#define RE_CHARACTER_CLASSES    1

#define POCSAG_REGEXP   "^POCSAG1200:\\s+Address:\\s+([0-9]+)\\s+Function:\\s+([0-9]+)\\s+Alpha:\\s+(.*)$"

void *read_input();
void process_input(const char* input);
void *keepalive();
static void broadcast(const struct wab_alert_msg *msg);
static void bc_init();
static void bc_keepalive();
static void bc_testalert();
static void dump_msg(FILE * stream, const struct wab_alert_msg *msg);

#endif
