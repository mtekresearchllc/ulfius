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
CFLAGS+=-c -Wall -I$(ULFIUS_INCLUDE) -I$(EXAMPLE_INCLUDE) -D_REENTRANT $(ADDITIONALFLAGS) $(CPPFLAGS)
LIBS=u_example.o -lc -lulfius -ljansson -L$(ULFIUS_LOCATION)


all: proxy

clean:
	rm -f *.o proxy

debug: ADDITIONALFLAGS=-DDEBUG -g -O0

debug: proxy

../../src/libulfius.so:
	cd $(ULFIUS_LOCATION) && $(MAKE) debug JANSSONFLAG=1 GNUTLSFLAG=1

proxy.o: proxy.c
	$(CC) $(CFLAGS) proxy.c -DDEBUG -g -O0

proxy: ../../src/libulfius.so proxy.o u_example.o
	$(CC) -o proxy proxy.o $(LIBS)

test: proxy
	LD_LIBRARY_PATH=$(ULFIUS_LOCATION):${LD_LIBRARY_PATH} ./proxy

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
