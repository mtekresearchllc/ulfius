/* private websocket extension collection
 * SPDX-License-Identifier: LGPL-2.1-only */
#include <stdint.h>
#include "u_list.h"
#include "u_memory.h"

void u_pointer_list_init(u_pointer_list_t *list)
{
        if (list) {
                list->size = 0;
                list->list = NULL;
        }
}

size_t u_pointer_list_size(const u_pointer_list_t *list)
{
        return list ? list->size : 0;
}

void *u_pointer_list_get_at(const u_pointer_list_t *list, size_t index)
{
        return list && index < list->size ? list->list[index] : NULL;
}

int u_pointer_list_append(u_pointer_list_t *list, void *item)
{
        void **resized;

        if (!list || list->size >= SIZE_MAX / sizeof(*resized)) {
                return 0;
        }
        resized = ulfius_realloc(list->list, (list->size + 1) * sizeof(*resized));
        if (!resized) {
                return 0;
        }
        list->list = resized;
        list->list[list->size++] = item;
        return 1;
}

void u_pointer_list_clean_free(u_pointer_list_t *list, void (*destroy)(void *))
{
        if (list) {
                while (list->size) {
                        if (destroy) {
                                destroy(list->list[--list->size]);
                        } else {
                                list->size--;
                        }
                }
                u_free(list->list);
                list->list = NULL;
        }
}
