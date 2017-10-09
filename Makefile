CC = gcc
CFLAGS = -Wall
DEPS = rs232.h
# OBJS = noncanonical.o writenoncanonical.o

all: noncanonical writenoncanonical

build/%: src/%.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

noncanonical: src/noncanonical.c src/rs232.c
	gcc $(CFLAGS) -o build/$@ $^

writenoncanonical: src/writenoncanonical.c src/rs232.c
	gcc $(CFLAGS) -o build/$@ $^
