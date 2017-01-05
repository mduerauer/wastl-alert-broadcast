#ifndef wab_h
#define wab_h

#define BUFLEN  1024

#define WAB_MSG_KEEPALIVE   1
#define WAB_MSG_ALERT       2
#define WAB_MSG_TEXT_LEN    256

#define SUBRIC_A    0
#define SUBRIC_B    1
#define SUBRIC_C    2
#define SUBRIC_D    3

#define DEFAULT_PORT 8888

typedef struct wab_alert_msg
{
    int     ts;
    int     msgtype;
    int     ric;
    int     subric;
    char    text[WAB_MSG_TEXT_LEN];
} wab_alert_msg;

#endif
