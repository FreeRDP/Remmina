## Process this file with automake to produce Makefile.in
plugindir = $(remmina_plugindir)

plugin_LTLIBRARIES = remmina-plugin-gkeyring.la

remmina_plugin_gkeyring_la_SOURCES = \
    remminaplugingkeyring.c

remmina_plugin_gkeyring_la_CFLAGS = @REMMINA_PLUGINS_GNOME_CFLAGS@

remmina_plugin_gkeyring_la_LDFLAGS = -avoid-version -module

remmina_plugin_gkeyring_la_LIBADD = @REMMINA_PLUGINS_GNOME_LIBS@

