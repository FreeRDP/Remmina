noinst_LTLIBRARIES = libvncclient.la

INCLUDES = $(GNUTLS_CFLAGS)

libvncclient_la_SOURCES = \
    libvncclient/cursor.c \
    libvncclient/listen.c \
    libvncclient/lzoconf.h \
    libvncclient/minilzo.c \
    libvncclient/minilzo.h \
    libvncclient/rfbproto.c \
    libvncclient/sockets.c \
    libvncclient/tls.c \
    libvncclient/tls.h \
    libvncclient/tight.c \
    libvncclient/vncviewer.c \
    libvncclient/zlib.c \
    libvncclient/zrle.c

libvncclient_la_LIBADD = $(GNUTLS_LIBS)

EXTRA_DIST = \
    libvncclient/corre.c \
    libvncclient/hextile.c \
    libvncclient/rre.c \
    libvncclient/ultra.c \
    libvncserver/d3des.c \
    libvncserver/d3des.h \
    libvncserver/vncauth.c \
    libvncserver/zywrletemplate.c

DISTCLEANFILES = \
    rfb/rfbconfig.h

noinst_HEADERS = \
    rfb/rfbclient.h \
    rfb/rfbconfig.h \
    rfb/rfbint.h \
    rfb/rfbproto.h \
    rfb/keysym.h

