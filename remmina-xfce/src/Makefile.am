## Process this file with automake to produce Makefile.in
plugindir = $(libdir)/xfce4/panel-plugins/

plugin_LTLIBRARIES = 				\
    libremmina-xfce-plugin.la

libremmina_xfce_plugin_la_SOURCES = remminaxfceplugin.c \
    remminaappletmenuitem.h remminaappletmenuitem.c \
    remminaavahi.h remminaavahi.c \
    remminaappletutil.h remminaappletutil.c

libremmina_xfce_plugin_la_CFLAGS = @REMMINA_XFCE_CFLAGS@ @AVAHI_CLIENT_CFLAGS@ \
    -DREMMINA_XFCE_LOCALEDIR=\"$(remmina_xfce_localedir)\" \
    -DGETTEXT_PACKAGE=\"$(GETTEXT_PACKAGE)\" \
    -DREMMINA_DATADIR=\"$(remmina_datadir)\"

libremmina_xfce_plugin_la_LDFLAGS = -avoid-version -module

libremmina_xfce_plugin_la_LIBADD = @REMMINA_XFCE_LIBS@ @AVAHI_CLIENT_LIBS@

desktopdir = $(datadir)/xfce4/panel-plugins
desktop_in_in_files = remmina-xfce-plugin.desktop.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in)
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

%.desktop.in: %.desktop.in.in
	sed -e "s,\@plugindir\@,$(plugindir),g" < $< > $@

EXTRA_DIST = $(desktop_in_in_files)

DISTCLEANFILES = $(desktop_DATA) $(desktop_in_files)

