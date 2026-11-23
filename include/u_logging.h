/* Ulfius application logging hooks
 * SPDX-License-Identifier: LGPL-2.1-only */
#ifndef ULFIUS_LOGGING_H
#define ULFIUS_LOGGING_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
        ULFIUS_LOG_INVALID = 0, // invalid severity
        ULFIUS_LOG_NONE,        // disable logging
        ULFIUS_LOG_ERROR,       // error diagnostics
        ULFIUS_LOG_WARNING,     // warning diagnostics
        ULFIUS_LOG_INFO,        // informational diagnostics
        ULFIUS_LOG_DEBUG        // debugging diagnostics
} ulfius_log_level_t;

/**
 * @brief receive a library diagnostic on the emitting thread
 *
 * @param user_data application context supplied during registration
 * @param level severity of this message
 * @param message borrowed text valid until the callback returns
 *
 * @return nothing
 */
typedef void (*ulfius_log_callback_t)(void *user_data,
        ulfius_log_level_t level, const char *message);

/**
 * @brief replace the process-wide logging hook and drain the previous hook
 *
 *        logging is disabled initially. enable with ERROR through DEBUG and a
 *        callback; disable with NONE, NULL, NULL. callbacks may run concurrently
 *        and must not re-enter Ulfius APIs except recursive logging, which is
 *        suppressed. the setter cannot be called from a callback. do not hold
 *        locks needed by callbacks when calling this blocking function. old
 *        context storage may be freed after a successful return. messages during
 *        replacement may be discarded. applications own this registration;
 *        dependent libraries must not replace it without application consent.
 *
 *        callbacks must return normally or use pthread_exit/deferred cancellation;
 *        longjmp and asynchronous cancellation are unsupported. neither logging
 *        function is async-signal-safe. a child of a multithreaded fork must exec
 *        before using logging. on any error the previous registration is retained.
 *
 * @param level maximum enabled verbosity
 * @param callback application hook or NULL when disabling
 * @param user_data opaque application context or NULL when disabling
 *
 * @return U_OK on success, U_ERROR_PARAMS for invalid arguments, or U_ERROR
 *         for initialization/synchronization failure or a call from a callback
 */
int ulfius_set_log_callback(ulfius_log_level_t level,
        ulfius_log_callback_t callback, void *user_data);

/**
 * @brief format and synchronously deliver a diagnostic to the application hook
 *
 *        INVALID/NONE severities, NULL format and disabled levels are ignored.
 *        callback text is borrowed; copy it to retain it. formatting/allocation
 *        failure delivers a fixed ERROR diagnostic without further allocation.
 *        logging uses private libc scratch memory, not application allocators.
 *        TLS initialization failure discards the message safely.
 *
 * @param level message severity
 * @param format printf format followed by its arguments
 *
 * @return nothing
 */
#if defined(__GNUC__) || defined(__clang__)
__attribute__((format(printf, 2, 3)))
#endif
void ulfius_log_message(ulfius_log_level_t level, const char *format, ...);

#ifdef __cplusplus
}
#endif
#endif
