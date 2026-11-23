/* Ulfius allocation domain
 * SPDX-License-Identifier: LGPL-2.1-only */
#include <pthread.h>
#include <stdlib.h>
#include "ulfius.h"

typedef struct {
        ulfius_malloc_t allocate;
        ulfius_realloc_t resize;
        ulfius_free_t release;
} u_allocator_t;

static pthread_mutex_t config_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t freeze_once = PTHREAD_ONCE_INIT;
static u_allocator_t configured = {malloc, realloc, free};
static u_allocator_t active = {malloc, realloc, free};
static int frozen;

static void u_freeze_allocator(void)
{
        pthread_mutex_lock(&config_lock);
        active = configured;
        frozen = 1;
        pthread_mutex_unlock(&config_lock);
}

int ulfius_set_alloc_funcs(ulfius_malloc_t malloc_fn,
                         ulfius_realloc_t realloc_fn, ulfius_free_t free_fn)
{
        int result = U_OK;

        if ((malloc_fn != NULL || realloc_fn != NULL || free_fn != NULL) &&
            (malloc_fn == NULL || realloc_fn == NULL || free_fn == NULL)) {
                return U_ERROR_PARAMS;
        }
        pthread_mutex_lock(&config_lock);
        if (frozen) {
                result = U_ERROR;
        } else {
                configured.allocate = malloc_fn != NULL ? malloc_fn : malloc;
                configured.resize = realloc_fn != NULL ? realloc_fn : realloc;
                configured.release = free_fn != NULL ? free_fn : free;
        }
        pthread_mutex_unlock(&config_lock);
        return result;
}

void ulfius_get_alloc_funcs(ulfius_malloc_t *malloc_fn,
                           ulfius_realloc_t *realloc_fn, ulfius_free_t *free_fn)
{
        pthread_mutex_lock(&config_lock);
        if (malloc_fn != NULL) {
                *malloc_fn = configured.allocate;
        }
        if (realloc_fn != NULL) {
                *realloc_fn = configured.resize;
        }
        if (free_fn != NULL) {
                *free_fn = configured.release;
        }
        pthread_mutex_unlock(&config_lock);
}

void *ulfius_malloc(size_t size)
{
        pthread_once(&freeze_once, u_freeze_allocator);
        return size != 0 ? active.allocate(size) : NULL;
}

void *ulfius_realloc(void *ptr, size_t size)
{
        pthread_once(&freeze_once, u_freeze_allocator);
        return size != 0 ? active.resize(ptr, size) : NULL;
}

void u_free(void *ptr)
{
        pthread_once(&freeze_once, u_freeze_allocator);
        if (ptr != NULL) {
                active.release(ptr);
        }
}
