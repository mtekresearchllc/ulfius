/* Ulfius example logging sink
 * SPDX-License-Identifier: MIT */
#include "include/u_example.h"
#include <stdio.h>
#include <time.h>

void u_example_log_callback(void *user_data, ulfius_log_level_t level,
        const char *message)
{
        FILE *stream = user_data;

        const char *names[] = { "INVALID", "NONE", "ERROR", "WARNING", "INFO", "DEBUG" };
        struct tm local;
        time_t now = time(NULL);
        char timestamp[32] = "unknown-time";

        if (localtime_r(&now, &local) != NULL) {
                strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &local);
        }
        fprintf(stream, "%s [%s] %s\n", timestamp,
                names[level >= ULFIUS_LOG_ERROR && level <= ULFIUS_LOG_DEBUG ? level : 0], message);
        fflush(stream);
}
