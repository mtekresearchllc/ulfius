/* Ulfius example logging sink
 * SPDX-License-Identifier: MIT */
#ifndef U_EXAMPLE_H
#define U_EXAMPLE_H

#include <ulfius.h>

/**
 * @brief write a diagnostic to an application-owned FILE stream
 *
 * @param user_data FILE pointer that remains open until hook unregistration
 * @param level diagnostic severity
 * @param message borrowed diagnostic text
 *
 * @return nothing
 */
void u_example_log_callback(void *user_data, ulfius_log_level_t level,
        const char *message);

#endif
