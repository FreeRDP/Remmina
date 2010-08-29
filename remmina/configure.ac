#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.60)
AC_INIT(remmina, 0.8.2)
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER([config.h])
AC_CONFIG_SRCDIR([src/remmina.c])

# Checks for programs.
AC_ISC_POSIX
AC_PROG_CC
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

GETTEXT_PACKAGE=remmina
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [The gettext package])

if test "x$USE_NLS" != "xno"; then
    ALL_LINGUAS="bg cs de el es eu fr hu id it lt nl pl pt_BR pt_PT ru tr uk zh_CN"
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

REMMINA_SUBDIRS="include src po desktop icons"

# Checks GTK+ version.
PKG_CHECK_MODULES(GTK, [
    glib-2.0 >= 2.20.0
    gtk+-2.0 >= 2.16.0
    gmodule-2.0
])
remmina_pkgs="glib-2.0 gtk+-2.0 gmodule-2.0"

# Check libavahi-ui
AC_ARG_ENABLE(avahi,
    [AS_HELP_STRING([--disable-avahi], [Disable Avahi])],
    [avahi_support=$enableval],
    [avahi_support="yes"]
)
if test "x$avahi_support" != "xno"; then
    PKG_CHECK_MODULES(AVAHI_UI,
        avahi-ui >= 0.6.22, [
            avahi_support="yes"
            remmina_pkgs="$remmina_pkgs avahi-ui"
            AC_DEFINE([HAVE_LIBAVAHI_UI], [1], [Define to 1 if you have the 'avahi-ui' library (-lavahi-ui).])
        ], avahi_support="no. libavahi-ui-dev required."
    )
else
    avahi_support="disabled."
fi

# Check libvte
AC_ARG_ENABLE(vte,
    [AS_HELP_STRING([--disable-vte], [Disable VTE (Terminal)])],
    [vte_support=$enableval],
    [vte_support="yes"]
)
if test "x$vte_support" != "xno"; then
    PKG_CHECK_MODULES(VTE,
        vte >= 0.12.0, [
            vte_support="yes"
            remmina_pkgs="$remmina_pkgs vte"
            AC_DEFINE([HAVE_LIBVTE], [1], [Define to 1 if you have the 'vte' library (-lvte).])
        ], vte_support="no. libvte-dev required."
    )
else
    vte_support="disabled."
fi

# Check libunique
AC_ARG_ENABLE(unique,
    [AS_HELP_STRING([--disable-unique], [Disable libunique])],
    [unique_support=$enableval],
    [unique_support="yes"]
)
if test "x$unique_support" != "xno"; then
    PKG_CHECK_MODULES(UNIQUE,
        unique-1.0 >= 1.0.8, [
            unique_support="yes"
            remmina_pkgs="$remmina_pkgs unique-1.0"
            AC_DEFINE([HAVE_LIBUNIQUE], [1], [Define to 1 if you have the 'unique' library (-lunique).])
        ], unique_support="no. libunique-dev required."
    )
else
    unique_support="disabled."
fi

# Check libpthread (if found I assume GThread uses pthread)
AC_CHECK_LIB(pthread, pthread_create, [
    AC_CHECK_HEADER(pthread.h, [
        pthread_support="yes"
        PKG_CHECK_MODULES(GTHREAD, [gthread-2.0 >= 2.14.0])
        remmina_pkgs="$remmina_pkgs gthread-2.0"
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
                LIBS="-lssh $LIBS"
            ], ssh_support="no. libssh-dev required.")
        ], ssh_support="no. libssh >= 0.4 required.")
    else
        ssh_support="no. Multi-threaded support required."
    fi
else
    ssh_support="disabled."
fi

# Check libgcrypt
AC_ARG_ENABLE(gcrypt,
    [AS_HELP_STRING([--disable-gcrypt], [Disable GCrypt])],
    [gcrypt_support=$enableval],
    [gcrypt_support="yes"]
)
if test "x$gcrypt_support" != "xno"; then
    AM_PATH_LIBGCRYPT(1.4.0,[
        gcrypt_support="yes"
        AC_DEFINE([HAVE_LIBGCRYPT], [1], [Define to 1 if you have the `gcrypt' library (-lgcrypt).])
    ],[
        gcrypt_support="no. libgcrypt-dev required."
    ])
else
    gcrypt_support="disabled."
fi

# Merge all PKG checks here
PKG_CHECK_MODULES(REMMINA, [
    $remmina_pkgs
])
AC_SUBST(REMMINA_CFLAGS)
AC_SUBST(REMMINA_LIBS)

AC_SUBST(REMMINA_SUBDIRS)

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

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([
Makefile
include/Makefile
src/Makefile
po/Makefile.in
desktop/Makefile
icons/Makefile
])

AC_OUTPUT

echo
echo "Remmina configure result:"
echo
echo "* NLS support: $nls_support"
echo "* Multi-threaded support: $pthread_support"
echo "* SSH support: $ssh_support"
echo "* Avahi support: $avahi_support"
echo "* Terminal support: $vte_support"
echo "* Encryption support: $gcrypt_support"
echo "* Unique-App support: $unique_support"
echo
echo "Type \"make\" to compile remmina."
echo

