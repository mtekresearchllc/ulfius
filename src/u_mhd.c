/* libmicrohttpd ownership boundaries
 * SPDX-License-Identifier: LGPL-2.1-only */
#include <stdlib.h>
#include "ulfius.h"
#include "u_string.h"
#include "u_mhd.h"

int u_mhd_copy_basic_auth(struct MHD_Connection *connection, struct _u_request *request)
{
        char *password = NULL;
        char *user = MHD_basic_auth_get_username_password(connection, &password);
        char *user_copy = u_strdup(user);
        char *password_copy = u_strdup(password);
        int failed = (user && !user_copy) || (password && !password_copy);

#if MHD_VERSION >= 0x00095600 && !defined(U_TEST_MHD_AUTH_FREE)
        MHD_free(user);
        MHD_free(password);
#else
        free(user);
        free(password);
#endif
        if (failed) {
                u_free(user_copy);
                u_free(password_copy);
                return U_ERROR_MEMORY;
        }
        u_free(request->auth_basic_user);
        u_free(request->auth_basic_password);
        request->auth_basic_user = user_copy;
        request->auth_basic_password = password_copy;
        return U_OK;
}

struct MHD_Response *u_mhd_response(size_t length, void **buffer)
{
        struct MHD_Response *response;

#if MHD_VERSION >= 0x00096100 && !defined(U_TEST_MHD_COPY)
        response = MHD_create_response_from_buffer_with_free_callback(
                length, *buffer, u_free);
        if (response == NULL) {
                u_free(*buffer);
        }
#else
        response = MHD_create_response_from_buffer(length, *buffer,
                                                   MHD_RESPMEM_MUST_COPY);
        u_free(*buffer);
#endif
        *buffer = NULL;
        return response;
}
