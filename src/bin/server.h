#ifndef server_h
#define server_h

#define EX_UNAVAILABLE  1

static void broadcast(const struct wab_alert_msg *msg);
static void bc_keepalive();
static void bc_testalert();

#endif
