AM_CFLAGS = -I$(top_srcdir)/src/bin

AM_CFLAGS += $(SQLITE3_CFLAGS)

bin_PROGRAMS = wabclient wabserver
wabclient_SOURCES = client.c client.h wab.h
wabserver_SOURCES = server.c server.h wab.h

wabserver_LDADD = $(SQLITE3_LIBS)
