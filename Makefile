# Makefile for he, a hex editor

CC := gcc
CFLAGS := -Wall -Wextra -Werror -pedantic --std=gnu99 -g
CPPFLAGS := -iquote . -iquote infos
LDFLAGS := -lncurses

INCLUDES := bits.h keynames.h infos/infos.h infos/z80.h infos/utf8.h infos/midi.h
LIBS := bits.o keynames.o infos/infos.o infos/z80.o infos/utf8.o infos/midi.o

all: he z80disasm

%.o: %.c %.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

he $(LIBS): info_choice.h

he: he.c $(INCLUDES) $(LIBS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $< $(LDFLAGS) -o $@ $(LIBS)

z80disasm: z80disasm.c bits.h infos/z80.h bits.o infos/z80.o
	$(CC) $(CFLAGS) $(CPPFLAGS) $< $(LDFLAGS) -o $@ bits.o infos/z80.o

