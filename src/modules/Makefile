CC = gcc
LD = ld

all: hello

hello: hello.c
	$(CC) -g -O -c hello.c
	$(LD) -shared -o hello.so hello.o

install:
	cp *.so ../../game/modules

clean:
	rm *.o *.so
