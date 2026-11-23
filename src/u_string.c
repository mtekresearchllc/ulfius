/* private string helpers
 * SPDX-License-Identifier: LGPL-2.1-only */
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "u_memory.h"
#include "u_string.h"

size_t u_strlen(const char *value)
{
        return value ? strlen(value) : 0;
}

int u_strnullempty(const char *value)
{
        return !value || !*value;
}

int u_strcmp(const char *left, const char *right)
{
        if (!left || !right) {
                return left == right ? 0 : (left ? -1 : 1);
        }
        return strcmp(left, right);
}

int u_strncmp(const char *left, const char *right, size_t count)
{
        if (!count) {
                return 0;
        }
        if (!left || !right) {
                return left == right ? 0 : (left ? -1 : 1);
        }
        return strncmp(left, right, count);
}

int u_strcasecmp(const char *left, const char *right)
{
        if (!left || !right) {
                return left == right ? 0 : (left ? -1 : 1);
        }
        return strcasecmp(left, right);
}

char *u_strcpy(char *dest, const char *source)
{
        return dest && source ? strcpy(dest, source) : NULL;
}

char *u_strncpy(char *dest, const char *source, size_t count)
{
        return dest && source ? strncpy(dest, source, count) : NULL;
}

const char *u_strchr(const char *value, int byte)
{
        return value ? strchr(value, byte) : NULL;
}

const char *u_strstr(const char *value, const char *needle)
{
        return value && needle ? strstr(value, needle) : NULL;
}

const char *u_strcasestr(const char *value, const char *needle)
{
        size_t index;

        if (!value || !needle) {
                return NULL;
        }
        do {
                for (index = 0; needle[index] && value[index] &&
                     tolower((unsigned char)needle[index]) ==
                     tolower((unsigned char)value[index]); index++) {
                }
                if (!needle[index]) {
                        return value;
                }
        } while (*value++);
        return NULL;
}

const char *u_strnstr(const char *value, const char *needle, size_t length)
{
        size_t needle_length;
        size_t index;

        if (!value || !needle) {
                return NULL;
        }
        needle_length = strlen(needle);
        if (!needle_length) {
                return value;
        }
        if (needle_length > length) {
                return NULL;
        }
        for (index = 0; index <= length - needle_length && value[index]; index++) {
                if (!strncmp(value + index, needle, needle_length)) {
                        return value + index;
                }
        }
        return NULL;
}

char *u_strndup(const char *source, size_t length)
{
        char *copy;

        if (!source || length == SIZE_MAX) {
                return NULL;
        }
        copy = ulfius_malloc(length + 1);
        if (copy) {
                memcpy(copy, source, length);
                copy[length] = '\0';
        }
        return copy;
}

char *u_strdup(const char *source)
{
        return source ? u_strndup(source, strlen(source)) : NULL;
}

char *u_vprintf(const char *format, va_list arguments)
{
        va_list copy;
        int length;
        int written;
        char *buffer;

        if (!format) {
                return NULL;
        }
        va_copy(copy, arguments);
        length = vsnprintf(NULL, 0, format, copy);
        va_end(copy);
        if (length < 0 || (size_t)length == SIZE_MAX) {
                return NULL;
        }
        buffer = ulfius_malloc((size_t)length + 1);
        if (!buffer) {
                return NULL;
        }
        va_copy(copy, arguments);
        written = vsnprintf(buffer, (size_t)length + 1, format, copy);
        va_end(copy);
        if (written != length) {
                u_free(buffer);
                return NULL;
        }
        return buffer;
}

char *u_printf(const char *format, ...)
{
        va_list arguments;
        char *buffer;

        va_start(arguments, format);
        buffer = u_vprintf(format, arguments);
        va_end(arguments);
        return buffer;
}

char *u_appendf(char *source, const char *format, ...)
{
        va_list arguments;
        char *suffix;
        char *result = NULL;
        size_t source_length;
        size_t suffix_length;

        va_start(arguments, format);
        suffix = u_vprintf(format, arguments);
        va_end(arguments);
        if (!source) {
                return suffix;
        }
        if (suffix) {
                source_length = u_strlen(source);
                suffix_length = strlen(suffix);
                if (source_length <= SIZE_MAX - suffix_length - 1) {
                        result = ulfius_malloc(source_length + suffix_length + 1);
                        if (result) {
                                if (source_length) {
                                        memcpy(result, source, source_length);
                                }
                                memcpy(result + source_length, suffix, suffix_length + 1);
                        }
                }
        }
        u_free(suffix);
        u_free(source);
        return result;
}

char *u_replace(const char *source, const char *needle, const char *replacement)
{
        const char *cursor;
        const char *match;
        char *result;
        char *output;
        size_t needle_length;
        size_t replacement_length;
        size_t length;
        size_t prefix;

        if (!source || !needle || !*needle || !replacement) {
                return NULL;
        }
        needle_length = strlen(needle);
        replacement_length = strlen(replacement);
        length = strlen(source);
        cursor = source;
        while ((match = strstr(cursor, needle))) {
                if (replacement_length >= needle_length) {
                        size_t increase = replacement_length - needle_length;
                        if (length >= SIZE_MAX - increase) {
                                return NULL;
                        }
                        length += increase;
                } else {
                        length -= needle_length - replacement_length;
                }
                cursor = match + needle_length;
        }
        result = ulfius_malloc(length + 1);
        if (!result) {
                return NULL;
        }
        output = result;
        cursor = source;
        while ((match = strstr(cursor, needle))) {
                prefix = (size_t)(match - cursor);
                memcpy(output, cursor, prefix);
                output += prefix;
                memcpy(output, replacement, replacement_length);
                output += replacement_length;
                cursor = match + needle_length;
        }
        strcpy(output, cursor);
        return result;
}
