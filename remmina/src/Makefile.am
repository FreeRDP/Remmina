## Process this file with automake to produce Makefile.in
bin_PROGRAMS = remmina

INCLUDES = @REMMINA_CFLAGS@ @LIBGCRYPT_CFLAGS@ \
    -I$(top_srcdir)/include \
    -DREMMINA_LOCALEDIR=\"$(remmina_localedir)\" \
    -DREMMINA_PLUGINDIR=\"$(remmina_plugindir)\" \
    -DGETTEXT_PACKAGE=\"$(GETTEXT_PACKAGE)\" \
    -DREMMINA_DATADIR=\"$(pkgdatadir)\"

remmina_SOURCES = remmina.c \
    remminamain.h remminamain.c \
    remminapublic.h remminapublic.c \
    remminafile.h remminafile.c \
    remminafilemanager.h remminafilemanager.c \
    remminafileeditor.h remminafileeditor.c \
    remminascrolledviewport.h remminascrolledviewport.c \
    remminaconnectionwindow.h remminaconnectionwindow.c \
    remminaprotocolwidget.h remminaprotocolwidget.c \
    remminainitdialog.h remminainitdialog.c \
    remminaabout.h remminaabout.c \
    remminapref.h remminapref.c \
    remminastringarray.h remminastringarray.c \
    remminastringlist.h remminastringlist.c \
    remminaprefdialog.h remminaprefdialog.c \
    remminassh.h remminassh.c \
    remminasshplugin.h remminasshplugin.c \
    remminachatwindow.h remminachatwindow.c \
    remminamarshals.h remminamarshals.c \
    remminaftpclient.h remminaftpclient.c \
    remminasftpclient.h remminasftpclient.c \
    remminasftpplugin.h remminasftpplugin.c \
    remminacrypt.h remminacrypt.c \
    remminachainbutton.h remminachainbutton.c \
    remminascaler.h remminascaler.c \
    remminawidgetpool.h remminawidgetpool.c \
    remminapluginmanager.h remminapluginmanager.c \
    remminalog.h remminalog.c \
    remminakeychooser.h remminakeychooser.c

remmina_LDADD = @REMMINA_LIBS@ @LIBGCRYPT_LIBS@

EXTRA_DIST = remminamarshals.list

