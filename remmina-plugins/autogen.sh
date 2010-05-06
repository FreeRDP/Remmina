#! /bin/sh

aclocal
autoheader --force
libtoolize -c --automake --force
intltoolize -c --automake --force
automake --add-missing --copy --include-deps
autoconf

rm -rf autom4te.cache

