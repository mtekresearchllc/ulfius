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
static pthread_t main_thread;
static int fail_auth;
static int auth_failures;
static int unexpected_callbacks;

static void *allocate(size_t size)
{
        block_t *block;
        if (fail_auth && size == 242 && !pthread_equal(main_thread, pthread_self())) {
                auth_failures++;
                return NULL;
        }
        block = malloc(sizeof(*block) + size);
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
        REQUIRE(!strcmp(u_map_get(request->map_url, "url key"), "url & value"));
        body = json_pack("{s:b}", "ok", 1);
        REQUIRE(body != NULL);
        REQUIRE(ulfius_set_json_body_response(response, 200, body) == U_OK);
        json_decref(body);
        return U_CALLBACK_COMPLETE;
}

static int unexpected_reply(const struct _u_request *request,
                            struct _u_response *response, void *unused)
{
        (void)request;
        (void)unused;
        unexpected_callbacks++;
        REQUIRE(ulfius_set_string_body_response(response, 200, "unexpected") == U_OK);
        return U_CALLBACK_COMPLETE;
}

static void test_auth_failure(int with_default)
{
        struct _u_instance instance;
        struct _u_request request;
        struct _u_response response;
        char username[242];

        memset(username, 'a', sizeof(username) - 1);
        username[sizeof(username) - 1] = 0;
        REQUIRE(ulfius_init_instance(&instance, 8080, NULL, NULL) == U_OK);
        REQUIRE(ulfius_add_endpoint_by_val(&instance, "GET", "/", NULL, 0,
                                          unexpected_reply, NULL) == U_OK);
        if (with_default) {
                REQUIRE(ulfius_set_default_endpoint(&instance, unexpected_reply, NULL) == U_OK);
        }
        fail_auth = 1;
        auth_failures = 0;
        REQUIRE(ulfius_start_framework(&instance) == U_OK);
        REQUIRE(ulfius_init_request(&request) == U_OK);
        REQUIRE(ulfius_init_response(&response) == U_OK);
        REQUIRE(ulfius_set_request_properties(&request, U_OPT_HTTP_VERB, "GET",
                U_OPT_HTTP_URL, "http://127.0.0.1:8080/", U_OPT_AUTH_BASIC,
                username, "secret", U_OPT_NONE) == U_OK);
        REQUIRE(ulfius_send_http_request(&request, &response) != U_OK);
        ulfius_clean_request(&request);
        ulfius_clean_response(&response);
        ulfius_stop_framework(&instance);
        fail_auth = 0;
        REQUIRE(auth_failures == 1 && unexpected_callbacks == 0);
        ulfius_clean_instance(&instance);
        REQUIRE(owned == 0);
}

int main(void)
{
        struct _u_instance instance;
        struct _u_request request;
        struct _u_response response;
        json_t *body;
        CURL *external;

        main_thread = pthread_self();
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
                "key space", "value & space", U_OPT_URL_PARAMETER,
                "url key", "url & value", U_OPT_NONE) == U_OK);
        REQUIRE(ulfius_send_http_request(&request, &response) == U_OK);
        REQUIRE(response.status == 200);
        body = ulfius_get_json_body_response(&response, NULL);
        REQUIRE(body && json_is_true(json_object_get(body, "ok")));
        json_decref(body);
        ulfius_clean_request(&request);
        ulfius_clean_response(&response);
        ulfius_stop_framework(&instance);
        ulfius_clean_instance(&instance);
        test_auth_failure(0);
        test_auth_failure(1);
        ulfius_global_close();
        REQUIRE(owned == 0);
        external = curl_easy_init();
        REQUIRE(external != NULL);
        curl_easy_cleanup(external);
        curl_global_cleanup();
        puts("HTTP allocation domains passed");
        return EXIT_SUCCESS;
}
