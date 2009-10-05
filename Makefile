# Makefile meant only for convenience and to provide a nicer
# interface for the newcomer.

all:
	make -f Makefile.cvs

install:
	make -f Makefile.cvs install

clean:
	rm -rf build

package:
	cd build ; make package_source
	mv build/*.tar.bz2 build/package/* package

test:
	build/src/test

.PHONY: all install clean package test

