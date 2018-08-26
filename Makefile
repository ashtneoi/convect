MAKEFLAGS += --no-builtin-rules --warn-undefined-variables
.SUFFIXES:
.DELETE_ON_ERROR:

.DEFAULT_GOAL := all
.PHONY: all clean


EXE := server client
SRC := $(EXE:%=%.c) fail.c

OBJ := $(SRC:%.c=%.o)

CC := gcc
CFLAGS := $(if $(DEBUG),-DDEBUG) \
	-std=c99 -pedantic -g -Wall -Wextra -Werror \
	$(if $(STRICT),-Wunused,-Wno-unused)
LFLAGS :=


all: $(EXE)

$(OBJ): %.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(EXE): %: %.o
	$(CC) -o $@ $^ $(LFLAGS)

clean:
	rm -f $(OBJ) $(EXE)


fail.o: common.h fail.h
server.o: common.h fail.h
client.o: common.h fail.h

server: fail.o
client: fail.o
