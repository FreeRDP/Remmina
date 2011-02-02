noinst_LTLIBRARIES = libvncclient.la

INCLUDES = $(GNUTLS_CFLAGS)

libvncclient_la_SOURCES = \
    libvncclient/cursor.c \
    libvncclient/listen.c \
    common/lzoconf.h \
    common/minilzo.c \
    common/minilzo.h \
    libvncclient/rfbproto.c \
    libvncclient/sockets.c \
    libvncclient/tls.c \
    libvncclient/tls.h \
    libvncclient/tight.c \
    libvncclient/vncviewer.c \
    libvncclient/zlib.c \
    libvncclient/zrle.c

libvncclient_la_CFLAGS = -I./common

libvncclient_la_LIBADD = $(GNUTLS_LIBS)

EXTRA_DIST = \
    libvncclient/corre.c \
    libvncclient/hextile.c \
    libvncclient/rre.c \
    libvncclient/ultra.c \
    common/d3des.c \
    common/d3des.h \
    common/vncauth.c \
    common/zywrletemplate.c

DISTCLEANFILES = \
    rfb/rfbconfig.h

noinst_HEADERS = \
    rfb/rfbclient.h \
    rfb/rfbconfig.h \
    rfb/rfbint.h \
    rfb/rfbproto.h \
    rfb/keysym.h

