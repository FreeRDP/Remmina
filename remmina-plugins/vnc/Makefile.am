## Process this file with automake to produce Makefile.in
plugindir = $(remmina_plugindir)

plugin_LTLIBRARIES = remmina-plugin-vnc.la

remmina_plugin_vnc_la_SOURCES = remminapluginvnc.c

remmina_plugin_vnc_la_CFLAGS = @VNC_CFLAGS@ @REMMINA_PLUGINS_CFLAGS@

remmina_plugin_vnc_la_LDFLAGS = -avoid-version -module

remmina_plugin_vnc_la_LIBADD = @VNC_LIBS@ @REMMINA_PLUGINS_LIBS@

appicon16emblemsdir = $(remmina_datadir)/icons/hicolor/16x16/emblems
appicon16emblems_DATA = \
    16x16/emblems/remmina-vnc.png \
    16x16/emblems/remmina-vnc-ssh.png

appicon22emblemsdir = $(remmina_datadir)/icons/hicolor/22x22/emblems
appicon22emblems_DATA = \
    22x22/emblems/remmina-vnc.png \
    22x22/emblems/remmina-vnc-ssh.png

EXTRA_DIST = \
    $(appicon16emblems_DATA) \
    $(appicon22emblems_DATA)

