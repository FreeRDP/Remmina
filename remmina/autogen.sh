#! /bin/sh

AM_INSTALLED_VERSION=$(automake --version | sed -e '2,$ d' -e 's/.* \([0-9]*\.[0-9]*\).*/\1/')

if [ "$AM_INSTALLED_VERSION" != "1.10" \
    -a "$AM_INSTALLED_VERSION" != "1.11" ];then
	echo
	echo "You must have automake 1.10 or 1.11 to prepare the GNU build system."
	exit 1
fi

if [ ! -d "m4" ]; then
    mkdir m4
fi

aclocal
autoheader --force
intltoolize -c --automake --force
automake --add-missing --copy --include-deps
autoconf

rm -rf autom4te.cache

