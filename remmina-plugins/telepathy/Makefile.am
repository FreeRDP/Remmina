## Process this file with automake to produce Makefile.in
plugindir = $(remmina_plugindir)

plugin_LTLIBRARIES = remmina-plugin-telepathy.la

remmina_plugin_telepathy_la_SOURCES = \
    remminaplugintelepathy.c \
    remminatphandler.c remminatphandler.h \
    remminatpchannelhandler.c remminatpchannelhandler.h

remmina_plugin_telepathy_la_CFLAGS = @TELEPATHY_CFLAGS@ @REMMINA_PLUGINS_CFLAGS@

remmina_plugin_telepathy_la_LDFLAGS = -avoid-version -module

remmina_plugin_telepathy_la_LIBADD = @TELEPATHY_LIBS@ @REMMINA_PLUGINS_LIBS@

clientdir = $(datarootdir)/telepathy/clients
client_DATA = Remmina.client

servicedir = $(datadir)/dbus-1/services
service_in_file = org.freedesktop.Telepathy.Client.Remmina.service.in
service_DATA = $(service_in_file:.service.in=.service)

$(service_DATA): $(service_in_file)
	sed -e "s,\@bindir\@,$(bindir),g" < $< > $@

EXTRA_DIST = \
    $(client_DATA) \
    $(service_in_file)

DISTCLEANFILES = $(service_DATA)

