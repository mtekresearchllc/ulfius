/* private json serialization with Ulfius-owned output
 * SPDX-License-Identifier: LGPL-2.1-only */
#include "u_json.h"
#ifndef U_DISABLE_JANSSON
#include <stdint.h>
#include <string.h>
#include "u_memory.h"

typedef struct {
        char   *data;
        size_t  length;
        size_t  capacity;
} u_json_buffer_t;

static int u_json_append(const char *data, size_t length, void *user_data)
{
        u_json_buffer_t *buffer = user_data;
        size_t required;
        size_t capacity;
        char *resized;

        if (length >= SIZE_MAX - buffer->length) {
                return -1;
        }
        required = buffer->length + length + 1;
        if (required > buffer->capacity) {
                capacity = buffer->capacity ? buffer->capacity : 256;
                while (capacity < required) {
                        capacity = capacity > SIZE_MAX / 2 ? required : capacity * 2;
                }
                resized = ulfius_realloc(buffer->data, capacity);
                if (!resized) {
                        return -1;
                }
                buffer->data = resized;
                buffer->capacity = capacity;
        }
        if (length) {
                memcpy(buffer->data + buffer->length, data, length);
        }
        buffer->length += length;
        buffer->data[buffer->length] = '\0';
        return 0;
}

char *u_json_dump(const json_t *value, size_t flags)
{
        u_json_buffer_t buffer = {NULL, 0, 0};

        if (!value || json_dump_callback(value, u_json_append, &buffer, flags) != 0) {
                u_free(buffer.data);
                return NULL;
        }
        return buffer.data;
}
#endif
