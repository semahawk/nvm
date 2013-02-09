CC = gcc
CFLAGS = -W -Wall -std=c99
OBJS = nvm.o grammar.o

.PHONY: all clean grammar clean distclean

all: lemon grammar nvm

lemon: lemon.o
	$(CC) $(CFLAGS) lemon.o -o lemon

lemon.o: lemon.c lempar.c
	$(CC) $(CFLAGS) -c lemon.c

grammar: grammar.o
grammar.o: lemon grammar.y
	./lemon -q grammar.y
	$(CC) $(CFLAGS) -c grammar.c

nvm: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o nvm

nvm.o: nvm.c nvm.h

clean:
	rm -f *.o
	rm -f grammar.c
	rm -f grammar.h

distclean: clean
	rm -f nvm
	rm -f lemon

