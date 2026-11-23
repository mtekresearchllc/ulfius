/* global lifecycle and first-use allocator regression tests
 * SPDX-License-Identifier: LGPL-2.1-only */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ulfius.h"
#ifndef U_DISABLE_CURL
#include <curl/curl.h>
#endif
#define REQUIRE(c) do { if (!(c)) abort(); } while (0)
static size_t hook_calls;
#ifndef U_DISABLE_CURL
static size_t init_calls;
static size_t close_calls;
static int fail_init = 1;

CURLcode curl_global_init(long flags)
{
        REQUIRE(flags == CURL_GLOBAL_DEFAULT);
        init_calls++;
        return fail_init ? CURLE_FAILED_INIT : CURLE_OK;
}

void curl_global_cleanup(void)
{
        close_calls++;
}
#endif

static void *allocate(size_t size)
{
        hook_calls++;
        return malloc(size);
}

static void *resize(void *ptr, size_t size)
{
        hook_calls++;
        return realloc(ptr, size);
}

static void release(void *ptr)
{
        hook_calls++;
        free(ptr);
}

int main(int argc, char **argv)
{
        REQUIRE(ulfius_set_alloc_funcs(allocate, resize, release) == U_OK);
        if (argc == 2) {
                if (!strcmp(argv[1], "malloc")) REQUIRE(ulfius_malloc(0) == NULL);
                else if (!strcmp(argv[1], "realloc")) REQUIRE(ulfius_realloc(NULL, 0) == NULL);
                else u_free(NULL);
                REQUIRE(hook_calls == 0);
                REQUIRE(ulfius_set_alloc_funcs(NULL, NULL, NULL) == U_ERROR);
                return EXIT_SUCCESS;
        }
        ulfius_global_close();
#ifndef U_DISABLE_CURL
        REQUIRE(ulfius_global_init() == U_ERROR);
        REQUIRE(init_calls == 1 && close_calls == 0);
        ulfius_global_close();
        REQUIRE(close_calls == 0);
        fail_init = 0;
#endif
        REQUIRE(ulfius_global_init() == U_OK);
        REQUIRE(ulfius_global_init() == U_OK);
        REQUIRE(ulfius_set_alloc_funcs(NULL, NULL, NULL) == U_ERROR);
        ulfius_global_close();
#ifndef U_DISABLE_CURL
        REQUIRE(init_calls == 2 && close_calls == 0);
#endif
        ulfius_global_close();
        ulfius_global_close();
#ifndef U_DISABLE_CURL
        REQUIRE(close_calls == 1);
#endif
        REQUIRE(ulfius_global_init() == U_OK);
        ulfius_global_close();
#ifndef U_DISABLE_CURL
        REQUIRE(init_calls == 3 && close_calls == 2);
#endif
        REQUIRE(hook_calls == 0);
        puts("runtime ownership contracts passed");
        return EXIT_SUCCESS;
}
