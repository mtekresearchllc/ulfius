/* private MHD ownership adapters
 * SPDX-License-Identifier: LGPL-2.1-only */
#ifndef ULFIUS_MHD_PRIVATE_H
#define ULFIUS_MHD_PRIVATE_H
#include "ulfius.h"
#if defined(__GNUC__)
#pragma GCC visibility push(hidden)
#endif
int u_mhd_copy_basic_auth(struct MHD_Connection *connection,
                          struct _u_request *request);
struct MHD_Response *u_mhd_response(size_t length, void **buffer);
#if defined(__GNUC__)
#pragma GCC visibility pop
#endif
#endif
