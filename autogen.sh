#! /bin/sh
mkdir -p build-aux
aclocal
autoconf -i -f
automake --add-missing --foreign -Wall
rm -r autom4te.cache
