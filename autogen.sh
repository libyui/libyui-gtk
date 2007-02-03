#!/bin/sh
# Run this to generate all the initial makefiles, etc.

aclocal $ACLOCAL_FLAGS || exit 1;
libtoolize --force --copy
autoheader
automake --gnu --add-missing --copy || exit 1;
autoconf || exit 1;

libdir=`pkg-config --print-errors --variable=libdir yast2-core`;

if test "z$PKG_CONFIG_PATH" = "z"; then
    export PKG_CONFIG_PATH="/opt/gnome/lib64/pkgconfig:/usr/lib64/pkgconfig;/opt/gnome/lib/pkgconfig:/usr/lib/pkgconfige"
fi

if test "z$@" = "z"; then
    ./configure --prefix=/usr --libdir=$libdir

elif test "x$NOCONFIGURE" = "x"; then
    ./configure "$@"

else
    echo "Skipping configure process."
fi
