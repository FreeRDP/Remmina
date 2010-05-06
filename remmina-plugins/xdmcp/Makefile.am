## Process this file with automake to produce Makefile.in
plugindir = $(remmina_plugindir)

plugin_LTLIBRARIES = remmina-plugin-xdmcp.la

remmina_plugin_xdmcp_la_SOURCES = remminapluginxdmcp.c

remmina_plugin_xdmcp_la_CFLAGS = @REMMINA_PLUGINS_CFLAGS@ \
    -DREMMINA_LOCALEDIR=\"$(remmina_localedir)\"

remmina_plugin_xdmcp_la_LDFLAGS = -avoid-version -module

remmina_plugin_xdmcp_la_LIBADD = @REMMINA_PLUGINS_LIBS@

appicon16emblemsdir = $(remmina_datadir)/icons/hicolor/16x16/emblems
appicon16emblems_DATA = \
    16x16/emblems/remmina-xdmcp.png \
    16x16/emblems/remmina-xdmcp-ssh.png

appicon22emblemsdir = $(remmina_datadir)/icons/hicolor/22x22/emblems
appicon22emblems_DATA = \
    22x22/emblems/remmina-xdmcp.png \
    22x22/emblems/remmina-xdmcp-ssh.png

EXTRA_DIST = \
    $(appicon16emblems_DATA) \
    $(appicon22emblems_DATA)

