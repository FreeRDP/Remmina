## Process this file with automake to produce Makefile.in
plugindir = $(remmina_plugindir)

plugin_LTLIBRARIES = remmina-plugin-nx.la

remmina_plugin_nx_la_SOURCES = \
    remminanxsession.c remminanxsession.h \
    remminapluginnx.c

remmina_plugin_nx_la_CFLAGS = @XKBFILE_CFLAGS@ @REMMINA_PLUGINS_CFLAGS@

remmina_plugin_nx_la_LDFLAGS = -avoid-version -module

remmina_plugin_nx_la_LIBADD = @SSH_LIBS@ @XKBFILE_LIBS@ @REMMINA_PLUGINS_LIBS@

appicon16emblemsdir = $(remmina_datadir)/icons/hicolor/16x16/emblems
appicon16emblems_DATA = \
    16x16/emblems/remmina-nx.png

appicon22emblemsdir = $(remmina_datadir)/icons/hicolor/22x22/emblems
appicon22emblems_DATA = \
    22x22/emblems/remmina-nx.png

EXTRA_DIST = \
    $(appicon16emblems_DATA) \
    $(appicon22emblems_DATA)

