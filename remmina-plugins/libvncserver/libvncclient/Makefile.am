INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/common

libvncclient_la_SOURCES=cursor.c listen.c rfbproto.c sockets.c vncviewer.c ../common/minilzo.c tls.c

noinst_HEADERS=../common/lzodefs.h ../common/lzoconf.h ../common/minilzo.h tls.h

rfbproto.o: rfbproto.c corre.c hextile.c rre.c tight.c zlib.c zrle.c ultra.c

EXTRA_DIST=corre.c hextile.c rre.c tight.c zlib.c zrle.c ultra.c tls.c

$(libvncclient_la_OBJECTS): ../rfb/rfbclient.h

lib_LTLIBRARIES=libvncclient.la


