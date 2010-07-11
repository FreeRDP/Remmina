#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.60)
AC_INIT(remmina-plugins, 0.8.1)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([common/remminaplugincommon.h])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_ISC_POSIX
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Disable optimization if we enable debug
AC_ARG_ENABLE(debug,
    [AS_HELP_STRING([--enable-debug], [Enable compiling in debug mode])], [
    if test "x$enableval" = "xyes"; then
        CFLAGS="-g -O0"
    fi]
)

# Only use -Wall if we have gcc
if test "x$GCC" = "xyes"; then
    if test -z "`echo "$CFLAGS" | grep "\-Wall" 2> /dev/null`" ; then
        CFLAGS="$CFLAGS -Wall"
    fi
fi

IT_PROG_INTLTOOL([0.35.0])

GETTEXT_PACKAGE=remmina-plugins
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [The gettext package])

if test "x$USE_NLS" != "xno"; then
    ALL_LINGUAS="bg de el es eu fr hu id it lt nl pl pt_BR pt_PT ru tr uk zh_CN"
    nls_support="yes"
else
    ALL_LINGUAS=""
    nls_support="disabled."
fi

AM_GLIB_GNU_GETTEXT

remmina_localedir='${datadir}/locale'
AC_SUBST(remmina_localedir)

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
CFLAGS="$CFLAGS -I${prefix}/include"
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

# Check libssh
AC_ARG_ENABLE(ssh,
    [AS_HELP_STRING([--disable-ssh], [Disable SSH])],
    [ssh_support=$enableval],
    [ssh_support="yes"]
)
if test "x$ssh_support" != "xno"; then
    if test "$pthread_support" = "yes"; then
        AC_CHECK_LIB(ssh, ssh_options_set, [
            AC_CHECK_HEADER(libssh/libssh.h, [
                ssh_support="yes"
                AC_DEFINE([HAVE_LIBSSH], [1], [Define to 1 if you have the `ssh' library (-lssh).])
                SSH_LIBS="-lssh"
                AC_SUBST(SSH_LIBS)
            ], ssh_support="no. libssh-dev required.")
        ], ssh_support="no. libssh >= 0.4 required.")
    else
        ssh_support="no. Multi-threaded support required."
    fi
else
    ssh_support="disabled."
fi

# Check libxkbfile
PKG_CHECK_MODULES(XKBFILE, [xkbfile], [xkbfile_support="yes"], [xkbfile_support="no. libxkbfile-dev required."])

# Check remmina executable
if test "x$prefix" = "xNONE"; then
    remminabin=$ac_default_prefix/bin/remmina
else
    remminabin=$prefix/bin/remmina
fi
AC_CHECK_FILE($remminabin,
    [remminabin_found="yes"],
    [remminabin_found="no. (If you are packaging the plugins, please make sure it uses the same prefix as remmina and ignore this)"])

REMMINA_PLUGINS_SUBDIRS="common po"

# Check RDP dependencies
AC_ARG_ENABLE(rdp,
    [AS_HELP_STRING([--disable-rdp], [Disable RDP])],
    [rdp_plugin=$enableval],
    [rdp_plugin="yes"]
)
if test "x$rdp_plugin" != "xno"; then
    PKG_CHECK_MODULES(RDP, [freerdp], [
        REMMINA_PLUGINS_SUBDIRS="$REMMINA_PLUGINS_SUBDIRS rdp"
    ], [
        rdp_plugin="no. freerdp-dev rquired."
    ])
else
    rdp_plugin="disabled."
fi

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
    AC_DEFINE([IPv6], [1], [Enable support for IPv6 in libvncclient])
    vnc_plugin="yes"
fi

# Check XDMCP dependencies
AC_ARG_ENABLE(xdmcp,
    [AS_HELP_STRING([--disable-xdmcp], [Disable XDMCP])],
    [xdmcp_plugin=$enableval],
    [xdmcp_plugin="yes"]
)
if test "x$xdmcp_plugin" != "xno"; then
    REMMINA_PLUGINS_SUBDIRS="$REMMINA_PLUGINS_SUBDIRS xdmcp"
else
    xdmcp_plugin="disabled."
fi

# Check NX dependencies
if test "x$ssh_support" = "xyes"; then
    if test "x$xkbfile_support" = "xyes"; then
        AC_ARG_ENABLE(nx,
            [AS_HELP_STRING([--disable-nx], [Disable NX])],
            [nx_plugin=$enableval],
            [nx_plugin="yes"]
        )
        if test "x$nx_plugin" != "xno"; then
            REMMINA_PLUGINS_SUBDIRS="$REMMINA_PLUGINS_SUBDIRS nx"
        else
            nx_plugin="disabled."
        fi
    else
        nx_plugin="no. libxkbfile required."
    fi
else
    nx_plugin="no. libssh required."
fi

# Check Telepathy dependencies
AC_ARG_ENABLE(telepathy,
    [AS_HELP_STRING([--disable-telepathy], [Disable Telepathy])],
    [telepathy_plugin=$enableval],
    [telepathy_plugin="yes"]
)
if test "x$telepathy_plugin" != "xno"; then
    PKG_CHECK_MODULES(TELEPATHY, [telepathy-glib >= 0.9.0], [
        REMMINA_PLUGINS_SUBDIRS="$REMMINA_PLUGINS_SUBDIRS telepathy"
    ], [
        telepathy_plugin="no. telepathy-glib-dev rquired."
    ])
else
    telepathy_plugin="disabled."
fi

# Checks for header files.
AC_CHECK_HEADERS_ONCE(
    netdb.h
    arpa/inet.h
    netinet/in.h
    sys/socket.h
    sys/un.h
    fcntl.h
    errno.h
)

AC_SUBST(REMMINA_PLUGINS_SUBDIRS)

AC_CONFIG_FILES([
Makefile
common/Makefile
po/Makefile.in
rdp/Makefile
libvncserver/Makefile
vnc/Makefile
xdmcp/Makefile
nx/Makefile
telepathy/Makefile
])

AC_OUTPUT

echo
echo "Remmina-Plugins configure result:"
echo
echo "* Remmina main program: $remminabin_found"
echo "* NLS support: $nls_support"
echo "* Multi-threaded support: $pthread_support"
echo "* SSH support: $ssh_support"
echo "* XKB support: $xkbfile_support"
echo
echo "* RDP plugin: $rdp_plugin"
echo "* VNC plugin: $vnc_plugin"
echo "* XDMCP plugin: $xdmcp_plugin"
echo "* NX plugin: $nx_plugin"
echo "* Telepathy plugin: $telepathy_plugin"
echo
echo "Type \"make\" to compile remmina-plugins."
echo

