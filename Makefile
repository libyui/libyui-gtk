all:
	make -f Makefile.cvs

clean:
	rm -rf build

install:
	make -f Makefile.cvs install

