/* private base64 encoding
 * SPDX-License-Identifier: LGPL-2.1-only */
#ifndef ULFIUS_BASE64_H
#define ULFIUS_BASE64_H
#include <stddef.h>
#include <stdint.h>

typedef struct {
        size_t   size;
        uint8_t *data;
} u_encoded_t;

#if defined(__GNUC__)
#pragma GCC visibility push(hidden)
#endif
// output capacity includes the terminating nul byte
int u_base64_encode(const uint8_t *source, size_t length, uint8_t *output,
                    size_t capacity, size_t *output_length);
int u_base64_encode_alloc(const uint8_t *source, size_t length, u_encoded_t *output);
#if defined(__GNUC__)
#pragma GCC visibility pop
#endif
#endif
