/* MHD allocation-domain and failure regression tests
 * SPDX-License-Identifier: LGPL-2.1-only */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "u_mhd.h"

#define REQUIRE(c) do { if (!(c)) abort(); } while (0)
typedef union {
        uint64_t magic;
        long double alignment;
} block_t;
static size_t owned;
static size_t mhd_frees;
static int fail_after = -1;
static int fail_response;
static void *response_buffer;
static void (*response_free)(void *);

static void *allocate(size_t size)
{
        block_t *block;
        if (fail_after == 0) return NULL;
        if (fail_after > 0) fail_after--;
        block = malloc(sizeof(*block) + size);
        REQUIRE(block != NULL);
        block->magic = UINT64_C(0xabc123abc123);
        owned++;
        return block + 1;
}

static void release(void *ptr)
{
        block_t *block = (block_t *)ptr - 1;
        REQUIRE(block->magic == UINT64_C(0xabc123abc123));
        block->magic = 0;
        owned--;
        free(block);
}

static void *resize(void *ptr, size_t size)
{
        REQUIRE(ptr == NULL);
        return allocate(size);
}

char *MHD_basic_auth_get_username_password(struct MHD_Connection *connection,
                                          char **password)
{
        char *user = malloc(5);
        (void)connection;
        *password = malloc(7);
        REQUIRE(user && *password);
        memcpy(user, "user", 5);
        memcpy(*password, "secret", 7);
        return user;
}

void MHD_free(void *ptr)
{
        if (ptr) mhd_frees++;
        free(ptr);
}

struct MHD_Response *MHD_create_response_from_buffer_with_free_callback(
        size_t size, void *buffer, MHD_ContentReaderFreeCallback callback)
{
        REQUIRE(size == 8);
        if (fail_response) return NULL;
        response_buffer = buffer;
        response_free = callback;
        return (struct MHD_Response *)&response_buffer;
}

struct MHD_Response *MHD_create_response_from_buffer(
        size_t size, void *buffer, enum MHD_ResponseMemoryMode mode)
{
        REQUIRE(mode == MHD_RESPMEM_MUST_COPY && size == 8 && buffer != NULL);
        if (fail_response) return NULL;
        return (struct MHD_Response *)&response_buffer;
}

int main(void)
{
        struct _u_request request = {0};
        struct MHD_Response *response;
        void *buffer;
        int failure;

        REQUIRE(ulfius_set_alloc_funcs(allocate, resize, release) == U_OK);
        for (failure = 0; failure < 2; failure++) {
                fail_after = failure;
                REQUIRE(u_mhd_copy_basic_auth(NULL, &request) == U_ERROR_MEMORY);
                REQUIRE(!request.auth_basic_user && !request.auth_basic_password);
                REQUIRE(owned == 0 && mhd_frees == (size_t)(failure + 1) * 2);
        }
        fail_after = -1;
        REQUIRE(u_mhd_copy_basic_auth(NULL, &request) == U_OK);
        REQUIRE(strcmp(request.auth_basic_user, "user") == 0);
        REQUIRE(strcmp(request.auth_basic_password, "secret") == 0);
        REQUIRE(mhd_frees == 6 && owned == 2);
        REQUIRE(u_mhd_copy_basic_auth(NULL, &request) == U_OK);
        REQUIRE(mhd_frees == 8 && owned == 2);
        u_free(request.auth_basic_user);
        u_free(request.auth_basic_password);
        for (failure = 0; failure < 2; failure++) {
                fail_response = failure;
                buffer = ulfius_malloc(8);
                response = u_mhd_response(8, &buffer);
                REQUIRE(buffer == NULL);
                REQUIRE((response == NULL) == failure);
                if (response_free != NULL) {
                        response_free(response_buffer);
                        response_free = NULL;
                }
                REQUIRE(owned == 0);
        }
        puts("MHD ownership contracts passed");
        return EXIT_SUCCESS;
}
