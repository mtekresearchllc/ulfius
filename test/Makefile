#
# Public domain, no copyright. Use at your own risk.
#

ULFIUS_INCLUDE=../include
ULFIUS_LOCATION=../src
ULFIUS_EXAMPLE_CALLBACK_COMPRESS=../example_callbacks/http_compression
ULFIUS_EXAMPLE_CALLBACK_FILE=../example_callbacks/static_compressed_inmemory_website
UTILITY_SOURCES=$(ULFIUS_LOCATION)/u_string.c $(ULFIUS_LOCATION)/u_token.c $(ULFIUS_LOCATION)/u_base64.c $(ULFIUS_LOCATION)/u_list.c $(ULFIUS_LOCATION)/u_json.c
ULFIUS_LIBRARY=$(ULFIUS_LOCATION)/libulfius.so
ULFIUS_SCRUTINIZE=$(wildcard $(ULFIUS_INCLUDE)/*.h $(ULFIUS_LOCATION)/*.c)
CC=gcc
CFLAGS+=-Wall -Werror -Wextra -D_REENTRANT -I$(ULFIUS_INCLUDE) -I../example_programs/include -DDEBUG -g -O0 $(CPPFLAGS)
LDFLAGS=-lc -L$(ULFIUS_LOCATION) -lulfius $(shell pkg-config --libs check) $(shell pkg-config --libs gnutls) $(shell pkg-config --libs libcurl) $(shell pkg-config --libs jansson) -lz -lpthread
# Use this LDFLAGS below if you don't have/need gnutls
#LDFLAGS=-lc -lulfius -lcheck -lpthread -lm -lrt -lsubunit -L$(ULFIUS_LOCATION)
# Alternative linker flags
#LDFLAGS=-lc -lulfius -lgnutls -lcheck -lpthread -lm -lrt -lsubunit -L$(ULFIUS_LOCATION)
VALGRIND_COMMAND=valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all
TARGET=u_map core framework websocket websocket_write_retry example_callbacks logging logging_faults utilities memory_runtime memory_threads memory_http memory_mhd_callback memory_mhd_copy memory_mhd_legacy_auth
VERBOSE=0
MEMCHECK=0
CERT=cert

all: test

clean:
	rm -f *.o $(TARGET) valgrind-*.txt *.log $(CERT)/*.crt $(CERT)/*.key

$(CERT)/server.key:
	./$(CERT)/create-cert.sh

$(ULFIUS_LIBRARY): $(ULFIUS_SCRUTINIZE)
	cd $(ULFIUS_LOCATION) && $(MAKE) debug

%: %.c
	$(CC) $(CFLAGS) $^ $(UTILITY_SOURCES) -o $@ $(LDFLAGS)

websocket_write_retry: websocket_write_retry.c $(ULFIUS_LOCATION)/u_websocket.c
	$(CC) $(CFLAGS) $< $(UTILITY_SOURCES) -o $@ $(LDFLAGS) -lmicrohttpd

http_compression_callback.o: $(ULFIUS_EXAMPLE_CALLBACK_COMPRESS)/http_compression_callback.c $(ULFIUS_EXAMPLE_CALLBACK_COMPRESS)/http_compression_callback.h
	$(CC) -c $(CFLAGS) -Wconversion $(ULFIUS_EXAMPLE_CALLBACK_COMPRESS)/http_compression_callback.c

static_compressed_inmemory_website_callback.o: $(ULFIUS_EXAMPLE_CALLBACK_FILE)/static_compressed_inmemory_website_callback.c $(ULFIUS_EXAMPLE_CALLBACK_FILE)/static_compressed_inmemory_website_callback.h
	$(CC) -c $(CFLAGS) -Wconversion $(ULFIUS_EXAMPLE_CALLBACK_FILE)/static_compressed_inmemory_website_callback.c

example_callbacks: example_callbacks.c u_example.o http_compression_callback.o static_compressed_inmemory_website_callback.o
	$(CC) -I$(ULFIUS_EXAMPLE_CALLBACK_COMPRESS) -I$(ULFIUS_EXAMPLE_CALLBACK_FILE) example_callbacks.c u_example.o http_compression_callback.o static_compressed_inmemory_website_callback.o $(UTILITY_SOURCES) -o example_callbacks $(LDFLAGS) $(CFLAGS)

test: $(ULFIUS_LIBRARY) $(CERT)/server.key $(TARGET) test_u_map test_core test_framework test_websocket test_websocket_write_retry test_example_callbacks test_logging test_logging_faults test_utilities test_memory_runtime test_memory_threads test_memory_http test_memory_mhd_callback test_memory_mhd_copy test_memory_mhd_legacy_auth

test_%: % $(ULFIUS_LIBRARY) $(CERT)/server.key
	@if [ "$(VERBOSE)" = "0" ] && [ "$(MEMCHECK)" = "0" ]; then \
		LD_LIBRARY_PATH=$(ULFIUS_LOCATION):${LD_LIBRARY_PATH} ./run_test.sh ./$^; \
	elif [ "$(MEMCHECK)" = "0" ]; then \
		LD_LIBRARY_PATH=$(ULFIUS_LOCATION):${LD_LIBRARY_PATH} ./$^ ; \
	else \
		LD_LIBRARY_PATH=$(ULFIUS_LOCATION):${LD_LIBRARY_PATH} $(VALGRIND_COMMAND) ./$^ 2>valgrind-$@.txt; \
	fi

check: test

u_example.o: ../example_programs/u_example.c ../example_programs/include/u_example.h
	$(CC) -c $(CFLAGS) ../example_programs/u_example.c -o $@

logging_faults: logging_faults.c $(ULFIUS_LOCATION)/u_logging.c
	$(CC) $(CFLAGS) $< $(UTILITY_SOURCES) -o $@ $(LDFLAGS)

MEMORY_MHD_SOURCES=memory_mhd.c $(ULFIUS_LOCATION)/u_mhd.c $(ULFIUS_LOCATION)/u_string.c $(ULFIUS_LOCATION)/u_memory.c
memory_mhd_callback: $(MEMORY_MHD_SOURCES)
	$(CC) $(CFLAGS) $^ -o $@ -lpthread
memory_mhd_copy: $(MEMORY_MHD_SOURCES)
	$(CC) $(CFLAGS) -DU_TEST_MHD_COPY $^ -o $@ -lpthread

test_memory_runtime: memory_runtime
	LD_LIBRARY_PATH=$(ULFIUS_LOCATION):$${LD_LIBRARY_PATH} ./memory_runtime
	LD_LIBRARY_PATH=$(ULFIUS_LOCATION):$${LD_LIBRARY_PATH} ./memory_runtime malloc
	LD_LIBRARY_PATH=$(ULFIUS_LOCATION):$${LD_LIBRARY_PATH} ./memory_runtime realloc
	LD_LIBRARY_PATH=$(ULFIUS_LOCATION):$${LD_LIBRARY_PATH} ./memory_runtime free

memory_mhd_legacy_auth: $(MEMORY_MHD_SOURCES)
	$(CC) $(CFLAGS) -DU_TEST_MHD_AUTH_FREE $^ -o $@ -lpthread
