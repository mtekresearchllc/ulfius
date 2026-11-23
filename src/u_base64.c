/* private base64 encoding
 * SPDX-License-Identifier: LGPL-2.1-only */
#include <stdint.h>
#include "u_base64.h"
#include "u_memory.h"

static int u_base64_length(size_t length, size_t *encoded)
{
        size_t groups = length / 3 + (length % 3 != 0);

        if (groups > (SIZE_MAX - 1) / 4) {
                return 0;
        }
        *encoded = groups * 4;
        return 1;
}

int u_base64_encode(const uint8_t *source, size_t length, uint8_t *output,
                    size_t capacity, size_t *output_length)
{
        static const char alphabet[] =
                "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        size_t encoded;
        size_t input = 0;
        size_t index = 0;
        uint32_t value;
        size_t remaining;

        if (output_length) {
                *output_length = 0;
        }
        if ((!source && length) || !output || !output_length ||
            !u_base64_length(length, &encoded) || capacity <= encoded) {
                return 0;
        }
        while (input < length) {
                remaining = length - input;
                value = (uint32_t)source[input++] << 16;
                if (remaining > 1) {
                        value |= (uint32_t)source[input++] << 8;
                }
                if (remaining > 2) {
                        value |= source[input++];
                }
                output[index++] = (uint8_t)alphabet[(value >> 18) & 63];
                output[index++] = (uint8_t)alphabet[(value >> 12) & 63];
                output[index++] = remaining > 1 ? (uint8_t)alphabet[(value >> 6) & 63] : '=';
                output[index++] = remaining > 2 ? (uint8_t)alphabet[value & 63] : '=';
        }
        output[index] = '\0';
        *output_length = index;
        return 1;
}

int u_base64_encode_alloc(const uint8_t *source, size_t length, u_encoded_t *output)
{
        size_t encoded;
        uint8_t *buffer;

        if (!output) {
                return 0;
        }
        output->size = 0;
        output->data = NULL;
        if ((!source && length) || !u_base64_length(length, &encoded)) {
                return 0;
        }
        buffer = ulfius_malloc(encoded + 1);
        if (!buffer) {
                return 0;
        }
        if (!u_base64_encode(source, length, buffer, encoded + 1, &output->size)) {
                u_free(buffer);
                return 0;
        }
        output->data = buffer;
        return 1;
}
