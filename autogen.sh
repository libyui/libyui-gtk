#!/bin/sh
# Run this to generate all the initial makefiles, etc.

aclocal $ACLOCAL_FLAGS || exit 1;
libtoolize --force --copy
autoheader
automake --gnu --add-missing --copy || exit 1;
autoconf || exit 1;

if test "z$@" = "z"; then
    ./configure --libdir=/usr/lib --prefix=/usr PKG_CONFIG_PATH=/opt/gnome/lib/pkgconfig:/usr/lib/pkgconfig;

elif test "x$NOCONFIGURE" = "x"; then
    ./configure "$@"

else
    echo "Skipping configure process."
fi
