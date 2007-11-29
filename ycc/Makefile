# Yast Control Center (Yast-GTK)

CC = g++
CFLAGS = -Wall `pkg-config gtk+-2.0 --cflags`
LIBS =`pkg-config gtk+-2.0 --libs`

all: ycc-gtk

ycc.o: ycc.cpp
	$(CC) $(CFLAGS) ycc.cpp -c -o ycc.o

ycc-gtk: ycc.o
	$(CC) $(LIBS) ycc.o -o ycc-gtk

clean:
	rm -f *.o *~ ycc-gtk
