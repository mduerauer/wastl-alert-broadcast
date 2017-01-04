#ifndef wab_h
#define wab_h

#define BUFLEN  255

#define WAB_MSG_KEEPALIVE   1
#define WAB_MSG_ALERT       2
#define WAB_MSG_TEXT_LEN    128

typedef struct wab_alert_msg
{
    int     ts;
    int     msgtype;
    int     ric;
    int     subric;
    char    text[WAB_MSG_TEXT_LEN];
} wab_alert_msg;

#endif
