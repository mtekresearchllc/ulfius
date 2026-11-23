/* private json serialization with Ulfius-owned output
 * SPDX-License-Identifier: LGPL-2.1-only */
#ifndef ULFIUS_JSON_H
#define ULFIUS_JSON_H
#include <stddef.h>
#include "ulfius-cfg.h"
#ifndef U_DISABLE_JANSSON
#include <jansson.h>
#if defined(__GNUC__)
#pragma GCC visibility push(hidden)
#endif
char *u_json_dump(const json_t *value, size_t flags);
#if defined(__GNUC__)
#pragma GCC visibility pop
#endif
#endif
#endif
