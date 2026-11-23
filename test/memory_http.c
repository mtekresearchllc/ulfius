/* real HTTP calls across independent allocation domains
 * SPDX-License-Identifier: LGPL-2.1-only */
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "ulfius.h"
#define REQUIRE(c) do { if (!(c)) abort(); } while (0)
typedef union {
        uint64_t magic;
        long double alignment;
} block_t;
static pthread_mutex_t count_lock = PTHREAD_MUTEX_INITIALIZER;
static size_t owned;

static void *allocate(size_t size)
{
        block_t *block = malloc(sizeof(*block) + size);
        REQUIRE(block != NULL);
        block->magic = UINT64_C(0x84795837);
        pthread_mutex_lock(&count_lock);
        owned++;
        pthread_mutex_unlock(&count_lock);
        return block + 1;
}

static void *resize(void *ptr, size_t size)
{
        block_t *block;
        if (!ptr) return allocate(size);
        block = (block_t *)ptr - 1;
        REQUIRE(block->magic == UINT64_C(0x84795837));
        block = realloc(block, sizeof(*block) + size);
        REQUIRE(block != NULL);
        return block + 1;
}

static void release(void *ptr)
{
        block_t *block = (block_t *)ptr - 1;
        REQUIRE(block->magic == UINT64_C(0x84795837));
        block->magic = 0;
        pthread_mutex_lock(&count_lock);
        REQUIRE(owned > 0);
        owned--;
        pthread_mutex_unlock(&count_lock);
        free(block);
}

static int reply(const struct _u_request *request, struct _u_response *response,
                 void *unused)
{
        json_t *body;
        (void)unused;
        REQUIRE(request->auth_basic_user && request->auth_basic_password);
        REQUIRE(!strcmp(request->auth_basic_user, "user"));
        REQUIRE(!strcmp(request->auth_basic_password, "secret"));
        REQUIRE(!strcmp(u_map_get(request->map_post_body, "key space"), "value & space"));
        body = json_pack("{s:b}", "ok", 1);
        REQUIRE(body != NULL);
        REQUIRE(ulfius_set_json_body_response(response, 200, body) == U_OK);
        json_decref(body);
        return U_CALLBACK_COMPLETE;
}

int main(void)
{
        struct _u_instance instance;
        struct _u_request request;
        struct _u_response response;
        json_t *body;
        CURL *external;

        REQUIRE(curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK);
        REQUIRE(ulfius_set_alloc_funcs(allocate, resize, release) == U_OK);
        REQUIRE(ulfius_global_init() == U_OK);
        REQUIRE(ulfius_init_instance(&instance, 8080, NULL, NULL) == U_OK);
        REQUIRE(ulfius_add_endpoint_by_val(&instance, "POST", "/", NULL, 0,
                                          reply, NULL) == U_OK);
        REQUIRE(ulfius_start_framework(&instance) == U_OK);
        REQUIRE(ulfius_init_request(&request) == U_OK);
        REQUIRE(ulfius_init_response(&response) == U_OK);
        REQUIRE(ulfius_set_request_properties(&request, U_OPT_HTTP_VERB, "POST",
                U_OPT_HTTP_URL, "http://127.0.0.1:8080/", U_OPT_AUTH_BASIC,
                "user", "secret", U_OPT_POST_BODY_PARAMETER,
                "key space", "value & space", U_OPT_NONE) == U_OK);
        REQUIRE(ulfius_send_http_request(&request, &response) == U_OK);
        REQUIRE(response.status == 200);
        body = ulfius_get_json_body_response(&response, NULL);
        REQUIRE(body && json_is_true(json_object_get(body, "ok")));
        json_decref(body);
        ulfius_clean_request(&request);
        ulfius_clean_response(&response);
        ulfius_stop_framework(&instance);
        ulfius_clean_instance(&instance);
        ulfius_global_close();
        REQUIRE(owned == 0);
        external = curl_easy_init();
        REQUIRE(external != NULL);
        curl_easy_cleanup(external);
        curl_global_cleanup();
        puts("HTTP allocation domains passed");
        return EXIT_SUCCESS;
}
