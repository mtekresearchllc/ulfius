/* Ulfius example logging sink
 * SPDX-License-Identifier: MIT */
#include "include/u_example.h"
#include <stdio.h>

void u_example_log_callback(void *user_data, ulfius_log_level_t level,
        const char *message)
{
        FILE *stream = user_data;

        fprintf(stream, "[%d] %s\n", (int)level, message);
        fflush(stream);
}
