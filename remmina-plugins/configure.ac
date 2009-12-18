#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.60)
AC_INIT(remmina-plugins, 0.7.99.1)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([common/remminaplugincommon.h])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_ISC_POSIX
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Only use -Wall if we have gcc
if test "x$GCC" = "xyes"; then
    if test -z "`echo "$CFLAGS" | grep "\-Wall" 2> /dev/null`" ; then
        CFLAGS="$CFLAGS -Wall"
    fi
fi

remmina_plugindir='${libdir}/remmina/plugins'
AC_SUBST(remmina_plugindir)

remmina_datadir='${datadir}/remmina'
AC_SUBST(remmina_datadir)

# Checks for libraries.
PKG_CHECK_MODULES(REMMINA_PLUGINS, [
    glib-2.0 >= 2.20.0
    gtk+-2.0 >= 2.16.0
])
AC_SUBST(REMMINA_PLUGINS_CFLAGS)
AC_SUBST(REMMINA_PLUGINS_LIBS)

# Checks for plugin development files
saved_CFLAGS=$CFLAGS
CFLAGS="$CFLAGS $REMMINA_PLUGINS_CFLAGS"
AC_CHECK_HEADER(remmina/plugin.h, [], [
    echo "ERROR: Remmina plugin development headers are not found."
    exit 1
], [
#include <gtk/gtk.h>
])
CFLAGS=$saved_CFLAGS

# Check libpthread
AC_CHECK_LIB(pthread, pthread_create, [
    AC_CHECK_HEADER(pthread.h, [
        pthread_support="yes"
        AC_DEFINE([HAVE_PTHREAD], [1], [Define to 1 if you have the `pthread' library (-lpthread).])
    ], pthread_support="no. libpthread-dev required.")
],[
    pthread_support="no. libpthread required."
])

# Check remmina executable
if test "x$prefix" = "xNONE"; then
    remminabin=$ac_default_prefix/bin/remmina
else
    remminabin=$prefix/bin/remmina
fi
AC_CHECK_FILE($remminabin,
    [remminabin_found="yes"],
    [remminabin_found="no. (If you are packaging the plugins, please make sure it uses the same prefix as remmina and ignore this)"])

REMMINA_PLUGINS_SUBDIRS="common"

# Check RDP dependencies
rdp_plugin="yes"
REMMINA_PLUGINS_SUBDIRS="$REMMINA_PLUGINS_SUBDIRS rdp"

# Check VNC dependencies
AC_ARG_ENABLE(vnc,
    [AS_HELP_STRING([--disable-vnc], [Disable VNC])],
    [vnc_plugin=$enableval],
    [vnc_plugin="yes"]
)
if test "x$vnc_plugin" != "xno"; then
    if test "x$vnc_plugin" = "xdl"; then
        AC_CHECK_LIB(vncclient, rfbGetClient, [
            AC_CHECK_HEADER(rfb/rfbclient.h, [
                vnc_plugin="link to libvncclient externally"
                VNC_CFLAGS=""
                VNC_LIBS="-lvncclient"
                AC_SUBST(VNC_CFLAGS)
                AC_SUBST(VNC_LIBS)
                AC_DEFINE([HAVE_LIBVNCCLIENT], [1], [Define to 1 if you have libvncclient.])
            ], vnc_plugin="no. libvncclient(libvncserver)-dev required.")
        ],[
            vnc_plugin="no. libvncclient(libvncserver) required."
        ])
    else
        vnc_plugin=""
    fi
else
    vnc_plugin="disabled."
fi
if test "x$vnc_plugin" = "x"; then
    AC_CHECK_LIB(z, deflate, , vnc_plugin="no. zlib required.")
fi
if test "x$vnc_plugin" = "x"; then
    AC_CHECK_HEADER(zlib.h, , vnc_plugin="no. zlib-dev required.")
fi
if test "x$vnc_plugin" = "x"; then
    AC_CHECK_LIB(jpeg, jpeg_CreateCompress, , vnc_plugin="no. libjpeg required.")
fi
if test "x$vnc_plugin" = "x"; then
    AC_CHECK_HEADER(jpeglib.h, , vnc_plugin="no. libjpeg-dev required.")
fi
if test "x$vnc_plugin" = "x"; then
    PKG_CHECK_MODULES(GNUTLS, gnutls >= 2.4.0, , vnc_plugin="no. libgnutls-dev required.")
fi
if test "x$vnc_plugin" = "x"; then
    AX_PREFIX_CONFIG_H([libvncserver/rfb/rfbconfig.h], [LIBVNCSERVER])
    REMMINA_PLUGINS_SUBDIRS="$REMMINA_PLUGINS_SUBDIRS libvncserver vnc"
    VNC_CFLAGS="-I../libvncserver $GNUTLS_CFLAGS"
    VNC_LIBS="../libvncserver/libvncclient.la -ljpeg -lz $GNUTLS_LIBS"
    AC_SUBST(VNC_CFLAGS)
    AC_SUBST(VNC_LIBS)
    AC_DEFINE([WITH_CLIENT_TLS], [1], [Enable support for gnutls in libvncclient])
    AC_DEFINE([HAVE_LIBVNCCLIENT], [1], [Define to 1 if you have libvncclient.])
    vnc_plugin="yes"
fi

# Check XDMCP dependencies
xdmcp_plugin="yes"
REMMINA_PLUGINS_SUBDIRS="$REMMINA_PLUGINS_SUBDIRS xdmcp"

# Checks for header files.
AC_CHECK_HEADERS_ONCE(
    netdb.h
    arpa/inet.h
    netinet/in.h
    sys/socket.h
    sys/un.h
    fcntl.h
    errno.h
    termios.h
)

AC_SUBST(REMMINA_PLUGINS_SUBDIRS)

AC_CONFIG_FILES([
Makefile
common/Makefile
rdp/Makefile
libvncserver/Makefile
vnc/Makefile
xdmcp/Makefile
])

AC_OUTPUT

echo
echo "Remmina-Plugins configure result:"
echo
echo "* Remmina main program: $remminabin_found"
echo "* Multi-threaded support: $pthread_support"
echo "* RDP plugin: $rdp_plugin"
echo "* VNC plugin: $vnc_plugin"
echo "* XDMCP plugin: $xdmcp_plugin"
echo
echo "Type \"make\" to compile remmina-plugins."
echo

