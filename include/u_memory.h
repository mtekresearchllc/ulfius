/* Ulfius memory ownership interface
 * SPDX-License-Identifier: LGPL-2.1-only */
#ifndef ULFIUS_MEMORY_H
#define ULFIUS_MEMORY_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void *(*ulfius_malloc_t)(size_t size);
typedef void *(*ulfius_realloc_t)(void *ptr, size_t size);
typedef void (*ulfius_free_t)(void *ptr);

/**
 * @brief configure allocation hooks before the first allocator use
 *        hooks and their state must remain valid for process lifetime
 *
 * @param malloc_fn allocator or NULL with both other hooks to restore libc
 * @param realloc_fn reallocator accepting NULL and preserving its input on failure
 * @param free_fn matching deallocator
 *
 * @return U_OK, U_ERROR_PARAMS for a partial tuple, or U_ERROR after first use
 */
int ulfius_set_alloc_funcs(ulfius_malloc_t malloc_fn,
                         ulfius_realloc_t realloc_fn, ulfius_free_t free_fn);

/**
 * @brief retrieve the configured allocation hooks without freezing them
 *        returned hooks become permanent only after first use
 *
 * @param malloc_fn optional allocator output
 * @param realloc_fn optional reallocator output
 * @param free_fn optional deallocator output
 *
 * @return nothing
 */
void ulfius_get_alloc_funcs(ulfius_malloc_t *malloc_fn,
                           ulfius_realloc_t *realloc_fn, ulfius_free_t *free_fn);

/**
 * @brief allocate storage using the permanent Ulfius allocator
 *        zero size returns NULL without invoking the hook but freezes configuration
 *
 * @param size number of bytes
 *
 * @return allocated storage or NULL on failure
 */
void *ulfius_malloc(size_t size);

/**
 * @brief resize storage using the permanent Ulfius allocator
 *        zero size returns NULL without freeing ptr or invoking the hook
 *        every call freezes the allocator configuration
 *
 * @param ptr storage from the same allocator or NULL
 * @param size number of bytes
 *
 * @return resized storage or NULL with original ownership retained on failure
 */
void *ulfius_realloc(void *ptr, size_t size);

/**
 * @brief release storage using the permanent Ulfius allocator
 *        NULL does not invoke the hook but still freezes configuration
 *
 * @param ptr storage owned by the Ulfius allocator or NULL
 *
 * @return nothing
 */
void u_free(void *ptr);

#ifdef __cplusplus
}
#endif
#endif
