/* race allocator configuration against concurrent first use
 * SPDX-License-Identifier: LGPL-2.1-only */
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "ulfius.h"
#define REQUIRE(c) do { if (!(c)) abort(); } while (0)
typedef union {
        uint64_t domain;
        long double alignment;
} block_t;
static pthread_barrier_t start;

static void *allocate_domain(size_t size, uint64_t domain)
{
        block_t *block = malloc(sizeof(*block) + size);
        REQUIRE(block != NULL);
        block->domain = domain;
        return block + 1;
}

static void *resize_domain(void *ptr, size_t size, uint64_t domain)
{
        block_t *block;
        if (!ptr) return allocate_domain(size, domain);
        block = (block_t *)ptr - 1;
        REQUIRE(block->domain == domain);
        block = realloc(block, sizeof(*block) + size);
        REQUIRE(block != NULL);
        return block + 1;
}

static void release_domain(void *ptr, uint64_t domain)
{
        block_t *block = (block_t *)ptr - 1;
        REQUIRE(block->domain == domain);
        free(block);
}

static void *allocate_a(size_t n) { return allocate_domain(n, 1); }
static void *allocate_b(size_t n) { return allocate_domain(n, 2); }
static void *resize_a(void *p, size_t n) { return resize_domain(p, n, 1); }
static void *resize_b(void *p, size_t n) { return resize_domain(p, n, 2); }
static void release_a(void *p) { release_domain(p, 1); }
static void release_b(void *p) { release_domain(p, 2); }

static void *worker(void *unused)
{
        size_t iteration;
        (void)unused;
        pthread_barrier_wait(&start);
        for (iteration = 0; iteration < 2000; iteration++) {
                void *ptr = ulfius_malloc(16);
                REQUIRE(ptr != NULL);
                ptr = ulfius_realloc(ptr, 32);
                REQUIRE(ptr != NULL);
                u_free(ptr);
        }
        return NULL;
}

int main(void)
{
        pthread_t workers[16];
        size_t index;
        int result;
        REQUIRE(ulfius_set_alloc_funcs(allocate_a, resize_a, release_a) == U_OK);
        REQUIRE(pthread_barrier_init(&start, NULL, 17) == 0);
        for (index = 0; index < 16; index++) {
                REQUIRE(pthread_create(&workers[index], NULL, worker, NULL) == 0);
        }
        pthread_barrier_wait(&start);
        for (index = 0; index < 1000; index++) {
                result = index % 2 ? ulfius_set_alloc_funcs(allocate_a, resize_a, release_a) :
                                     ulfius_set_alloc_funcs(allocate_b, resize_b, release_b);
                REQUIRE(result == U_OK || result == U_ERROR);
        }
        for (index = 0; index < 16; index++) {
                REQUIRE(pthread_join(workers[index], NULL) == 0);
        }
        pthread_barrier_destroy(&start);
        REQUIRE(ulfius_set_alloc_funcs(NULL, NULL, NULL) == U_ERROR);
        puts("concurrent allocator freeze passed");
        return EXIT_SUCCESS;
}
