## Process this file with automake to produce Makefile.in
plugindir = $(remmina_plugindir)

plugin_LTLIBRARIES = remmina-plugin-rdp.la

remmina_plugin_rdp_la_SOURCES = \
    remminapluginrdp.c remminapluginrdp.h \
    remminapluginrdpui.c remminapluginrdpui.h \
    remminapluginrdpev.c remminapluginrdpev.h

remmina_plugin_rdp_la_CFLAGS = @REMMINA_PLUGINS_CFLAGS@ @RDP_CFLAGS@

remmina_plugin_rdp_la_LDFLAGS = -avoid-version -module

remmina_plugin_rdp_la_LIBADD = @REMMINA_PLUGINS_LIBS@ @RDP_LIBS@

appicon16emblemsdir = $(remmina_datadir)/icons/hicolor/16x16/emblems
appicon16emblems_DATA = \
    16x16/emblems/remmina-rdp.png \
    16x16/emblems/remmina-rdp-ssh.png

appicon22emblemsdir = $(remmina_datadir)/icons/hicolor/22x22/emblems
appicon22emblems_DATA = \
    22x22/emblems/remmina-rdp.png \
    22x22/emblems/remmina-rdp-ssh.png

EXTRA_DIST = \
    $(appicon16emblems_DATA) \
    $(appicon22emblems_DATA)

