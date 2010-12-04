## Process this file with automake to produce Makefile.in
bin_PROGRAMS = remmina-applet

INCLUDES = @REMMINA_GNOME_CFLAGS@ @AVAHI_CLIENT_CFLAGS@ \
    -DREMMINA_GNOME_LOCALEDIR=\"$(remmina_gnome_localedir)\" \
    -DGETTEXT_PACKAGE=\"$(GETTEXT_PACKAGE)\" \
    -DREMMINA_DATADIR=\"$(remmina_datadir)\"

remmina_applet_SOURCES = remminaapplet.c \
    remminaappletmenuitem.h remminaappletmenuitem.c \
    remminaappletmenu.h remminaappletmenu.c \
    remminaavahi.h remminaavahi.c \
    remminaappletutil.h remminaappletutil.c

remmina_applet_LDADD = @REMMINA_GNOME_LIBS@ @AVAHI_CLIENT_LIBS@

server_in_files = Remmina_Applet.server.in.in

serverdir = $(libdir)/bonobo/servers
server_DATA = $(server_in_files:.server.in.in=.server)
$(server_in_files:.server.in.in=.server.in): $(server_in_files)
	sed -e "s|\@BINDIR\@|$(bindir)|" -e "s|\@REMMINA_PIXMAPSDIR\@|$(remmina_pixmapsdir)|" $< > $@

@INTLTOOL_SERVER_RULE@

EXTRA_DIST = $(server_in_files)
CLEANFILES = $(server_DATA) $(server_DATA).in

