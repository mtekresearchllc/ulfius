/* private string helpers
 * SPDX-License-Identifier: LGPL-2.1-only */
#ifndef ULFIUS_STRING_H
#define ULFIUS_STRING_H

#include <stddef.h>
#include <stdarg.h>

#if defined(__GNUC__)
#pragma GCC visibility push(hidden)
#define U_STRING_FORMAT(a, b) __attribute__((format(printf, a, b)))
#else
#define U_STRING_FORMAT(a, b)
#endif

size_t u_strlen(const char *value);
int u_strnullempty(const char *value);
int u_strcmp(const char *left, const char *right);
int u_strncmp(const char *left, const char *right, size_t count);
int u_strcasecmp(const char *left, const char *right);
char *u_strcpy(char *dest, const char *source);
char *u_strncpy(char *dest, const char *source, size_t count);
const char *u_strchr(const char *value, int byte);
const char *u_strstr(const char *value, const char *needle);
const char *u_strcasestr(const char *value, const char *needle);
const char *u_strnstr(const char *value, const char *needle, size_t length);
char *u_strdup(const char *source);
// copy exactly length bytes and append a terminator, including embedded nul bytes
char *u_strndup(const char *source, size_t length);
char *u_vprintf(const char *format, va_list arguments);
char *u_printf(const char *format, ...) U_STRING_FORMAT(1, 2);
// consume source on every path, after evaluating any arguments aliasing source
char *u_appendf(char *source, const char *format, ...) U_STRING_FORMAT(2, 3);
char *u_replace(const char *source, const char *needle, const char *replacement);

#if defined(__GNUC__)
#pragma GCC visibility pop
#endif
#endif
