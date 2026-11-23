/* private websocket extension collection
 * SPDX-License-Identifier: LGPL-2.1-only */
#ifndef ULFIUS_LIST_H
#define ULFIUS_LIST_H
#include <stddef.h>

#ifndef ULFIUS_POINTER_LIST_TYPE
#define ULFIUS_POINTER_LIST_TYPE
typedef struct _u_pointer_list u_pointer_list_t;
#endif
struct _u_pointer_list {
        size_t  size;
        void  **list;
};

#if defined(__GNUC__)
#pragma GCC visibility push(hidden)
#endif
void u_pointer_list_init(u_pointer_list_t *list);
size_t u_pointer_list_size(const u_pointer_list_t *list);
void *u_pointer_list_get_at(const u_pointer_list_t *list, size_t index);
int u_pointer_list_append(u_pointer_list_t *list, void *item);
void u_pointer_list_clean_free(u_pointer_list_t *list, void (*destroy)(void *));
#if defined(__GNUC__)
#pragma GCC visibility pop
#endif
#endif
