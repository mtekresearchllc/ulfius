/* allocator and private utility regression tests
 * SPDX-License-Identifier: LGPL-2.1-only */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ulfius.h"
#include "u_memory.h"
#include "u_string.h"
#include "u_token.h"
#include "u_base64.h"
#include "u_list.h"
#include "u_json.h"

#define REQUIRE(condition) do { \
        if (!(condition)) { \
                fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
                abort(); \
        } \
} while (0)

typedef union {
        struct {
                uint64_t magic;
                size_t length;
        } record;
        long double alignment;
        void *pointer_alignment;
} test_block_t;

static size_t live_blocks;
static size_t hook_calls;
static int fail_after = -1;
static const uint64_t block_magic = UINT64_C(0xb2a478f104cc195e);

static int allocation_fails(void)
{
        hook_calls++;
        if (fail_after == 0) {
                return 1;
        }
        if (fail_after > 0) {
                fail_after--;
        }
        return 0;
}

static void *test_malloc(size_t size)
{
        test_block_t *block;

        if (allocation_fails() || size > SIZE_MAX - sizeof(*block)) {
                return NULL;
        }
        block = malloc(sizeof(*block) + size);
        if (!block) {
                return NULL;
        }
        block->record.magic = block_magic;
        block->record.length = size;
        live_blocks++;
        return block + 1;
}

static void test_free(void *pointer)
{
        test_block_t *block;

        if (pointer) {
                block = (test_block_t *)pointer - 1;
                REQUIRE(block->record.magic == block_magic);
                block->record.magic = 0;
                REQUIRE(live_blocks > 0);
                live_blocks--;
                free(block);
        }
}

static void *test_realloc(void *pointer, size_t size)
{
        test_block_t *block;
        test_block_t *resized;

        if (!pointer) {
                return test_malloc(size);
        }
        block = (test_block_t *)pointer - 1;
        REQUIRE(block->record.magic == block_magic);
        if (allocation_fails() || size > SIZE_MAX - sizeof(*block)) {
                return NULL;
        }
        resized = realloc(block, sizeof(*block) + size);
        if (!resized) {
                return NULL;
        }
        resized->record.length = size;
        return resized + 1;
}

static void test_memory_contract(void)
{
        ulfius_malloc_t malloc_fn;
        ulfius_realloc_t realloc_fn;
        ulfius_free_t free_fn;
        void *pointer;
        size_t calls;

        REQUIRE(ulfius_set_alloc_funcs(test_malloc, NULL, test_free) == U_ERROR_PARAMS);
        REQUIRE(ulfius_set_alloc_funcs(NULL, NULL, NULL) == U_OK);
        REQUIRE(ulfius_set_alloc_funcs(test_malloc, test_realloc, test_free) == U_OK);
        ulfius_get_alloc_funcs(&malloc_fn, &realloc_fn, &free_fn);
        REQUIRE(malloc_fn == test_malloc && realloc_fn == test_realloc && free_fn == test_free);
        ulfius_get_alloc_funcs(NULL, NULL, NULL);
        calls = hook_calls;
        REQUIRE(ulfius_malloc(0) == NULL);
        REQUIRE(hook_calls == calls);
        REQUIRE(ulfius_set_alloc_funcs(NULL, NULL, NULL) == U_ERROR);
        pointer = ulfius_malloc(8);
        REQUIRE(pointer != NULL);
        calls = hook_calls;
        REQUIRE(ulfius_realloc(pointer, 0) == NULL);
        u_free(NULL);
        REQUIRE(hook_calls == calls && live_blocks == 1);
        fail_after = 0;
        REQUIRE(ulfius_realloc(pointer, 32) == NULL);
        REQUIRE(live_blocks == 1);
        fail_after = -1;
        u_free(pointer);
        REQUIRE(live_blocks == 0);
}

static void test_strings(void)
{
        char *copy;
        char *result;
        const char binary[] = {'a', '\0', 'b'};
        char buffer[8] = "abc";
        int fail;

        REQUIRE(u_strlen(NULL) == 0 && u_strnullempty(NULL));
        REQUIRE(u_strcmp(NULL, NULL) == 0);
        REQUIRE(u_strcmp(NULL, "") > 0 && u_strcmp("", NULL) < 0);
        REQUIRE(u_strncmp(NULL, "x", 0) == 0);
        REQUIRE(u_strcasecmp("AbC", "aBc") == 0);
        REQUIRE(u_strcpy(buffer, NULL) == NULL && !strcmp(buffer, "abc"));
        REQUIRE(u_strchr(NULL, 'x') == NULL && u_strstr("x", NULL) == NULL);
        REQUIRE(!strcmp(u_strcasestr("AbCd", "bc"), "bCd"));
        REQUIRE(u_strnstr("abc", "bc", 2) == NULL);
        REQUIRE(!strcmp(u_strnstr("abc", "bc", 3), "bc"));
        REQUIRE(u_strndup("x", SIZE_MAX) == NULL);
        copy = u_strndup(binary, sizeof(binary));
        REQUIRE(copy && !memcmp(copy, binary, sizeof(binary)) && copy[3] == '\0');
        u_free(copy);
        result = u_printf("%s-%d", "test", 17);
        REQUIRE(result && !strcmp(result, "test-17"));
        u_free(result);
        copy = u_strdup("abc");
        result = u_appendf(copy, "%s", copy);
        REQUIRE(result && !strcmp(result, "abcabc"));
        u_free(result);
        for (fail = 0; fail < 2; fail++) {
                copy = u_strdup("owned");
                REQUIRE(copy != NULL);
                fail_after = fail;
                REQUIRE(u_appendf(copy, "%s", copy) == NULL);
                fail_after = -1;
                REQUIRE(live_blocks == 0);
        }
        copy = u_strdup("owned");
        REQUIRE(u_appendf(copy, NULL) == NULL && live_blocks == 0);
        result = u_replace("a b  c", " ", "%20");
        REQUIRE(result && !strcmp(result, "a%20b%20%20c"));
        u_free(result);
        result = u_replace("abab", "ab", "");
        REQUIRE(result && !*result);
        u_free(result);
        REQUIRE(u_replace("a", "", "b") == NULL);
}

static void test_tokens(void)
{
        char **parts = NULL;
        const char *values[] = {" a ", "BB", NULL};
        char white[] = " \tword\r\n";
        char slashes[] = "///path///";
        int fail;

        REQUIRE(u_split_string("a::b::::", "::", &parts) == 4);
        REQUIRE(!strcmp(parts[0], "a") && !strcmp(parts[1], "b"));
        REQUIRE(!*parts[2] && !*parts[3] && parts[4] == NULL);
        u_free_string_array(parts);
        REQUIRE(u_split_string("", ",", &parts) == 1 && !*parts[0]);
        u_free_string_array(parts);
        REQUIRE(u_split_string("a,b", ",", NULL) == 2);
        REQUIRE(u_split_string(NULL, ",", &parts) == 0 && parts == NULL);
        REQUIRE(u_split_string("a", "", &parts) == 0 && parts == NULL);
        REQUIRE(u_string_array_has_trimmed_value(values, " a ") == 1);
        REQUIRE(u_string_array_has_trimmed_value(values, "bb") == 0);
        REQUIRE(u_string_array_has_value(values, "BB") == 1);
        REQUIRE(!strcmp(u_trimwhitespace(white), "word"));
        REQUIRE(!strcmp(u_trimcharacter(slashes, '/'), "path"));
        for (fail = 0; fail < 4; fail++) {
                fail_after = fail;
                REQUIRE(u_split_string("a,b,c", ",", &parts) == 0);
                fail_after = -1;
                REQUIRE(parts == NULL && live_blocks == 0);
        }
}

static void test_base64(void)
{
        static const char *plain[] = {"", "f", "fo", "foo", "foob", "fooba", "foobar"};
        static const char *encoded[] = {"", "Zg==", "Zm8=", "Zm9v", "Zm9vYg==", "Zm9vYmE=", "Zm9vYmFy"};
        uint8_t buffer[32];
        size_t length;
        size_t index;
        u_encoded_t result;

        for (index = 0; index < sizeof(plain) / sizeof(*plain); index++) {
                REQUIRE(u_base64_encode((const uint8_t *)plain[index], strlen(plain[index]),
                                        buffer, sizeof(buffer), &length));
                REQUIRE(length == strlen(encoded[index]) && !strcmp((const char *)buffer, encoded[index]));
                REQUIRE(u_base64_encode_alloc((const uint8_t *)plain[index], strlen(plain[index]), &result));
                REQUIRE(result.size == length && !strcmp((const char *)result.data, encoded[index]));
                u_free(result.data);
        }
        REQUIRE(!u_base64_encode((const uint8_t *)"foo", 3, buffer, 4, &length));
        REQUIRE(length == 0);
        REQUIRE(!u_base64_encode((const uint8_t *)"x", SIZE_MAX, buffer, sizeof(buffer), &length));
        REQUIRE(!u_base64_encode(NULL, 1, buffer, sizeof(buffer), &length));
        fail_after = 0;
        REQUIRE(!u_base64_encode_alloc((const uint8_t *)"foo", 3, &result));
        fail_after = -1;
        REQUIRE(result.data == NULL && result.size == 0);
}

static void test_lists(void)
{
        u_pointer_list_t list;
        void *first = ulfius_malloc(1);
        void *second = ulfius_malloc(1);
        void **storage;

        REQUIRE(first && second);
        u_pointer_list_init(&list);
        REQUIRE(u_pointer_list_append(&list, first));
        storage = list.list;
        fail_after = 0;
        REQUIRE(!u_pointer_list_append(&list, second));
        fail_after = -1;
        REQUIRE(list.list == storage && list.size == 1);
        REQUIRE(u_pointer_list_get_at(&list, 0) == first);
        REQUIRE(u_pointer_list_get_at(&list, 1) == NULL);
        REQUIRE(u_pointer_list_append(&list, second));
        REQUIRE(u_pointer_list_size(&list) == 2);
        u_pointer_list_clean_free(&list, u_free);
        REQUIRE(list.size == 0 && list.list == NULL && live_blocks == 0);
        list.size = SIZE_MAX;
        REQUIRE(!u_pointer_list_append(&list, NULL));
        list.size = 0;
        u_pointer_list_clean_free(&list, NULL);
}

#ifndef U_DISABLE_JANSSON
static size_t json_blocks;
static const uint64_t json_magic = UINT64_C(0x974dfcc5aaffc331);

static void *test_json_malloc(size_t size)
{
        test_block_t *block;

        if (size > SIZE_MAX - sizeof(*block)) {
                return NULL;
        }
        block = malloc(sizeof(*block) + size);
        if (!block) {
                return NULL;
        }
        block->record.magic = json_magic;
        json_blocks++;
        return block + 1;
}

static void test_json_free(void *pointer)
{
        test_block_t *block;

        if (pointer) {
                block = (test_block_t *)pointer - 1;
                REQUIRE(block->record.magic == json_magic && json_blocks > 0);
                json_blocks--;
                free(block);
        }
}

static void test_json_ownership(void)
{
        json_t *value;
        char *reference;
        char *result;
        char large[2048];
        int fail;

        json_set_alloc_funcs(test_json_malloc, test_json_free);
        memset(large, 'x', sizeof(large) - 1);
        large[sizeof(large) - 1] = '\0';
        value = json_pack("{s:s,s:[i,b,n],s:s}", "text", "quotes\"\n", "nested", 17, 1,
                          "large", large);
        REQUIRE(value != NULL);
        reference = json_dumps(value, JSON_COMPACT);
        REQUIRE(reference != NULL);
        result = u_json_dump(value, JSON_COMPACT);
        REQUIRE(result && !strcmp(result, reference));
        u_free(result);
        for (fail = 0; fail < 5; fail++) {
                fail_after = fail;
                result = u_json_dump(value, JSON_COMPACT);
                fail_after = -1;
                if (result) {
                        REQUIRE(!strcmp(result, reference));
                        u_free(result);
                }
                REQUIRE(live_blocks == 0);
        }
        {
                struct _u_request request;
                struct _u_response response;
#ifndef U_DISABLE_WEBSOCKET
                struct _websocket_manager manager = {0};
#endif
                REQUIRE(ulfius_init_request(&request) == U_OK);
                REQUIRE(ulfius_init_response(&response) == U_OK);
                REQUIRE(ulfius_set_json_body_request(&request, value) == U_OK);
                REQUIRE(ulfius_set_json_body_response(&response, 200, value) == U_OK);
                REQUIRE(request.binary_body_length == strlen(reference));
                REQUIRE(response.binary_body_length == strlen(reference));
                REQUIRE(!memcmp(request.binary_body, reference, strlen(reference)));
                REQUIRE(!memcmp(response.binary_body, reference, strlen(reference)));
                fail_after = 0;
                REQUIRE(ulfius_set_json_body_request(&request, value) == U_ERROR_MEMORY);
                REQUIRE(ulfius_set_json_body_response(&response, 200, value) == U_ERROR_MEMORY);
                REQUIRE(!request.binary_body && request.binary_body_length == 0);
                REQUIRE(!response.binary_body && response.binary_body_length == 0);
#ifndef U_DISABLE_WEBSOCKET
                REQUIRE(ulfius_websocket_send_json_message(&manager, value) == U_ERROR_MEMORY);
#endif
                fail_after = -1;
                ulfius_clean_request(&request);
                ulfius_clean_response(&response);
                REQUIRE(live_blocks == 0);
        }
        test_json_free(reference);
        json_decref(value);
        REQUIRE(json_blocks == 0);
        REQUIRE(u_json_dump(NULL, JSON_COMPACT) == NULL);
}
#endif

int main(void)
{
        test_memory_contract();
        test_strings();
        test_tokens();
        test_base64();
        test_lists();
#ifndef U_DISABLE_JANSSON
        test_json_ownership();
#endif
        REQUIRE(live_blocks == 0);
        puts("memory and utility contracts passed");
        return 0;
}
