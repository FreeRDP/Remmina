#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(remmina-xfce, 0.6.99.4)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/remminaxfceplugin.c])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

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

AC_SUBST(ACLOCAL_AMFLAGS, "$ACLOCAL_FLAGS -I m4")

IT_PROG_INTLTOOL([0.35.0])

GETTEXT_PACKAGE=remmina-xfce
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [The gettext package])

if test "x$USE_NLS" != "xno"; then
    ALL_LINGUAS="de el es it nl pl ru zh_CN"
    nls_support="yes"
else
    ALL_LINGUAS=""
    nls_support="disabled."
fi

AM_GLIB_GNU_GETTEXT

remmina_xfce_localedir='${datadir}/locale'
AC_SUBST(remmina_xfce_localedir)

remmina_datadir='${datadir}/remmina'
AC_SUBST(remmina_datadir)

# Checks for libraries.
PKG_CHECK_MODULES(REMMINA_XFCE, [
    glib-2.0 >= 2.20.0
    gtk+-2.0 >= 2.16.0
    libxfce4util-1.0 >= 4.3.99.2
    libxfce4panel-1.0 >= 4.3.99.2
])
AC_SUBST(REMMINA_XFCE_LIBS)
AC_SUBST(REMMINA_XFCE_CFLAGS)

# Check libavahi-client
AC_ARG_ENABLE(avahi,
    [AS_HELP_STRING([--disable-avahi], [Disable Avahi])],
    [avahi_support=$enableval],
    [avahi_support="yes"]
)
if test "x$avahi_support" != "xno"; then
    PKG_CHECK_MODULES(AVAHI_CLIENT,
        avahi-client >= 0.6.22, [
            avahi_support="yes"
            AC_SUBST(AVAHI_CLIENT_LIBS)
            AC_SUBST(AVAHI_CLIENT_CFLAGS)
            AC_DEFINE([HAVE_LIBAVAHI_CLIENT], [1], [Define to 1 if you have the 'avahi-client' library (-lavahi-client).])
        ], avahi_support="no. libavahi-client-dev required."
    )
else
    avahi_support="disabled."
fi

# Check remmina executable
if test "x$prefix" = "xNONE"; then
    remminabin=$ac_default_prefix/bin/remmina
else
    remminabin=$prefix/bin/remmina
fi
AC_CHECK_FILE($remminabin,
    [remminabin_found="yes"],
    [remminabin_found="no. (If you are packaging remmina-xfce, please make sure it uses the same prefix as remmina and ignore this)"])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([
Makefile
src/Makefile
po/Makefile.in
])

AC_OUTPUT

echo
echo "Remmina-Xfce configure result:"
echo
echo "* Remmina main program: $remminabin_found"
echo "* NLS support: $nls_support"
echo "* Avahi support: $avahi_support"
echo
echo "Type \"make\" to compile remmina-xfce."
echo

