# Makefile for he, a hex editor

CC := gcc
CFLAGS := -Wall -Wextra -Werror -pedantic --std=gnu99 -g
LDFLAGS := -lncurses

INCLUDES := bits.h keynames.h infos.h
LIBS := bits.o keynames.o infos.o

all: he

%.o: %.c %.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

he: he.c $(INCLUDES) $(LIBS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $< $(LDFLAGS) -o $@ $(LIBS)

