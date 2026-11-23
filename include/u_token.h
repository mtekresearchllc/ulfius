/* private token helpers
 * SPDX-License-Identifier: LGPL-2.1-only */
#ifndef ULFIUS_TOKEN_H
#define ULFIUS_TOKEN_H
#include <stddef.h>
#if defined(__GNUC__)
#pragma GCC visibility push(hidden)
#endif
size_t u_split_string(const char *value, const char *separator, char ***output);
void u_free_string_array(char **values);
int u_string_array_has_value(const char **values, const char *needle);
int u_string_array_has_trimmed_value(const char **values, const char *needle);
char *u_trimcharacter(char *value, char byte);
const char *u_trimwhitespace(char *value);
#if defined(__GNUC__)
#pragma GCC visibility pop
#endif
#endif
