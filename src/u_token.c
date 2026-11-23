/* private token helpers
 * SPDX-License-Identifier: LGPL-2.1-only */
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include "u_memory.h"
#include "u_string.h"
#include "u_token.h"

void u_free_string_array(char **values)
{
        size_t index;

        if (values) {
                for (index = 0; values[index]; index++) {
                        u_free(values[index]);
                }
                u_free(values);
        }
}

size_t u_split_string(const char *value, const char *separator, char ***output)
{
        const char *cursor;
        const char *match;
        size_t count = 1;
        size_t separator_length;
        size_t index;
        char **values;

        if (output) {
                *output = NULL;
        }
        if (!value || !separator || !*separator) {
                return 0;
        }
        separator_length = strlen(separator);
        cursor = value;
        while ((match = strstr(cursor, separator))) {
                if (count == SIZE_MAX - 1) {
                        return 0;
                }
                count++;
                cursor = match + separator_length;
        }
        if (!output) {
                return count;
        }
        if (count >= SIZE_MAX / sizeof(*values)) {
                return 0;
        }
        values = ulfius_malloc((count + 1) * sizeof(*values));
        if (!values) {
                return 0;
        }
        memset(values, 0, (count + 1) * sizeof(*values));
        cursor = value;
        for (index = 0; index < count; index++) {
                match = strstr(cursor, separator);
                values[index] = u_strndup(cursor, match ? (size_t)(match - cursor) : strlen(cursor));
                if (!values[index]) {
                        u_free_string_array(values);
                        return 0;
                }
                if (match) {
                        cursor = match + separator_length;
                }
        }
        *output = values;
        return count;
}

char *u_trimcharacter(char *value, char byte)
{
        size_t length;

        if (!value) {
                return NULL;
        }
        while (*value && *value == byte) {
                value++;
        }
        length = strlen(value);
        while (length && value[length - 1] == byte) {
                value[--length] = '\0';
        }
        return value;
}

const char *u_trimwhitespace(char *value)
{
        size_t length;

        if (!value) {
                return NULL;
        }
        while (isspace((unsigned char)*value)) {
                value++;
        }
        length = strlen(value);
        while (length && isspace((unsigned char)value[length - 1])) {
                value[--length] = '\0';
        }
        return value;
}

int u_string_array_has_value(const char **values, const char *needle)
{
        size_t index;

        if (!values || !needle) {
                return 0;
        }
        for (index = 0; values[index]; index++) {
                if (!strcmp(values[index], needle)) {
                        return 1;
                }
        }
        return 0;
}

static void u_trim_span(const char **value, size_t *length)
{
        while (**value && isspace((unsigned char)**value)) {
                (*value)++;
        }
        *length = strlen(*value);
        while (*length && isspace((unsigned char)(*value)[*length - 1])) {
                (*length)--;
        }
}

int u_string_array_has_trimmed_value(const char **values, const char *needle)
{
        const char *value;
        size_t needle_length;
        size_t length;
        size_t index;

        if (!values || !needle) {
                return 0;
        }
        u_trim_span(&needle, &needle_length);
        for (index = 0; values[index]; index++) {
                value = values[index];
                u_trim_span(&value, &length);
                if (length == needle_length && !memcmp(value, needle, length)) {
                        return 1;
                }
        }
        return 0;
}
