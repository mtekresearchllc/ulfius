#
# Example program
#
# Makefile used to build the software
#
# Copyright 2015-2022 Nicolas Mora <mail@babelouest.org>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the MIT License
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
CC=gcc
ULFIUS_LOCATION=../../src
ULFIUS_INCLUDE=../../include
EXAMPLE_INCLUDE=../include
CFLAGS+=-c -Wall -D_REENTRANT $(ADDITIONALFLAGS) -I$(ULFIUS_INCLUDE) -I$(EXAMPLE_INCLUDE) $(CPPFLAGS)
LIBS=u_example.o -lc -ljansson -lulfius -L$(ULFIUS_LOCATION)


all: client server mail

clean:
	rm -f *.o client server mail

debug: ADDITIONALFLAGS=-DDEBUG -g -O0

debug: client server mail

../../src/libulfius.so:
	cd $(ULFIUS_LOCATION) && $(MAKE) debug GNUTLSFLAG=1

client.o: client.c ../../src/libulfius.so
	$(CC) $(CFLAGS) client.c -DDEBUG -g -O0

client: client.o u_example.o
	$(CC) -o client client.o $(LIBS)

server.o: server.c ../../src/libulfius.so
	$(CC) $(CFLAGS) server.c -DDEBUG -g -O0

server: server.o u_example.o
	$(CC) -o server server.o $(LIBS)

mail.o: mail.c ../../src/libulfius.so
	$(CC) $(CFLAGS) mail.c -DDEBUG -g -O0

mail: mail.o u_example.o
	$(CC) -o mail mail.o $(LIBS)

u_example.o: ../u_example.c ../include/u_example.h
	$(CC) $(CFLAGS) ../u_example.c -o $@

ULFIUS_SUPPORT=$(if $(wildcard $(ULFIUS_LOCATION)/u_string.c),$(ULFIUS_LOCATION),../../utility_support)
CFLAGS+=-I$(ULFIUS_SUPPORT)
LIBS+=u_string.o u_token.o
u_example.o: u_string.o u_token.o
u_string.o: $(ULFIUS_SUPPORT)/u_string.c
	$(CC) $(CFLAGS) -c $< -o $@
u_token.o: $(ULFIUS_SUPPORT)/u_token.c
	$(CC) $(CFLAGS) -c $< -o $@
