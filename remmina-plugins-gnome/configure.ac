#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.60)
AC_INIT(remmina-plugins-gnome, 0.9.99.1)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/remminaplugingkeyring.c])
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

# Checks for libraries.
PKG_CHECK_MODULES(REMMINA_PLUGINS_GNOME, [
    glib-2.0 >= 2.20.0
    gtk+-2.0 >= 2.16.0
    gnome-keyring-1 >= 2.28.0
])
AC_SUBST(REMMINA_PLUGINS_GNOME_LIBS)
AC_SUBST(REMMINA_PLUGINS_GNOME_CFLAGS)

# Check remmina executable
if test "x$prefix" = "xNONE"; then
    remminabin=$ac_default_prefix/bin/remmina
else
    remminabin=$prefix/bin/remmina
fi
AC_CHECK_FILE($remminabin,
    [remminabin_found="yes"],
    [remminabin_found="no. (If you are packaging remmina-plugins-gnome, please make sure it uses the same prefix as remmina and ignore this)"])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([
Makefile
src/Makefile
])

AC_OUTPUT

echo
echo "remmina-plugins-gnome configure result:"
echo
echo "* Remmina main program: $remminabin_found"
echo
echo "Type \"make\" to compile remmina-plugins-gnome."
echo

