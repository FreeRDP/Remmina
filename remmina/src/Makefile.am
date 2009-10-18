## Process this file with automake to produce Makefile.in
bin_PROGRAMS = remmina

INCLUDES = @REMMINA_CFLAGS@ @LIBGCRYPT_CFLAGS@ \
    -I$(top_builddir)/libvncserver \
    -DREMMINA_LOCALEDIR=\"$(remmina_localedir)\" \
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
    remminaplug.h remminaplug.c \
    remminaplug_rdp.h remminaplug_rdp.c \
    remminaplug_vnc.h remminaplug_vnc.c \
    remminainitdialog.h remminainitdialog.c \
    remminaabout.h remminaabout.c \
    remminapref.h remminapref.c \
    remminastringarray.h remminastringarray.c \
    remminastringlist.h remminastringlist.c \
    remminaprefdialog.h remminaprefdialog.c \
    remminassh.h remminassh.c \
    remminachatwindow.h remminachatwindow.c \
    remminamarshals.h remminamarshals.c \
    remminaftpwindow.h remminaftpwindow.c \
    remminasftpwindow.h remminasftpwindow.c \
    remminacrypt.h remminacrypt.c \
    remminachainbutton.h remminachainbutton.c \
    remminascaler.h remminascaler.c \
    remminaplug_xdmcp.h remminaplug_xdmcp.c \
    remminawidgetpool.h remminawidgetpool.c

remmina_LDADD = @REMMINA_LIBS@ @LIBGCRYPT_LIBS@ \
    $(top_builddir)/libvncserver/libvncclient.la

EXTRA_DIST = remminamarshals.list

