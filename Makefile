CC = gcc
CFLAGS = -W -Wall -std=c99
OBJS = example.o nvm.o grammar.o

.PHONY: all grammar clean distclean

all: lemon grammar example

lemon: lemon.o
	$(CC) $(CFLAGS) lemon.o -o lemon

lemon.o: lemon.c lempar.c
	$(CC) $(CFLAGS) -c lemon.c

grammar: grammar.o
grammar.o: lemon grammar.y
	./lemon -q grammar.y
	$(CC) $(CFLAGS) -c grammar.c

example: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o example

example.o: example.c
	$(CC) $(CFLAGS) -c example.c

nvm.o: nvm.c nvm.h
	$(CC) $(CFLAGS) -c nvm.c

clean:
	rm -f *.o
	rm -f grammar.c
	rm -f grammar.h

distclean: clean
	rm -f example
	rm -f lemon

