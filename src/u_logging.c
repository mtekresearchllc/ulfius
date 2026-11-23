/* Ulfius application logging hooks
 * SPDX-License-Identifier: LGPL-2.1-only */
#include <ulfius.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
        ulfius_log_level_t     level;
        ulfius_log_callback_t  callback;
        void                  *user_data;
} u_log_hook_t;

typedef struct {
        u_log_hook_t  hook;
        char          *allocated;
} u_log_call_t;

static pthread_mutex_t u_log_setter_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t u_log_dispatch_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t u_log_drained = PTHREAD_COND_INITIALIZER;
static pthread_once_t u_log_key_once = PTHREAD_ONCE_INIT;
static pthread_key_t u_log_key;
static int u_log_key_error = 1;
static char u_log_active_marker;
static size_t u_log_in_flight;
static u_log_hook_t u_log_hook = { ULFIUS_LOG_NONE, NULL, NULL };

static void u_log_init_key(void)
{
        u_log_key_error = pthread_key_create(&u_log_key, NULL);
}

static int u_log_prepare_key(void)
{
        return pthread_once(&u_log_key_once, u_log_init_key) == 0 &&
                u_log_key_error == 0;
}

int ulfius_set_log_callback(ulfius_log_level_t level,
        ulfius_log_callback_t callback, void *user_data)
{
        u_log_hook_t previous;
        int cancel_state;
        int result = U_OK;

        if (!((level == ULFIUS_LOG_NONE && callback == NULL && user_data == NULL) ||
                (level >= ULFIUS_LOG_ERROR && level <= ULFIUS_LOG_DEBUG && callback != NULL))) {
                return U_ERROR_PARAMS;
        }
        if (!u_log_prepare_key() || pthread_getspecific(u_log_key) != NULL) {
                return U_ERROR;
        }
        if (pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &cancel_state) != 0) {
                return U_ERROR;
        }
        if (pthread_mutex_lock(&u_log_setter_lock) != 0) {
                pthread_setcancelstate(cancel_state, NULL);
                return U_ERROR;
        }
        if (pthread_mutex_lock(&u_log_dispatch_lock) != 0) {
                pthread_mutex_unlock(&u_log_setter_lock);
                pthread_setcancelstate(cancel_state, NULL);
                return U_ERROR;
        }
        previous = u_log_hook;
        u_log_hook.callback = NULL;
        while (u_log_in_flight != 0) {
                if (pthread_cond_wait(&u_log_drained, &u_log_dispatch_lock) != 0) {
                        result = U_ERROR;
                        break;
                }
        }
        if (result == U_OK) {
                u_log_hook.level = level;
                u_log_hook.callback = callback;
                u_log_hook.user_data = user_data;
        } else {
                u_log_hook = previous;
        }
        pthread_mutex_unlock(&u_log_dispatch_lock);
        pthread_mutex_unlock(&u_log_setter_lock);
        pthread_setcancelstate(cancel_state, NULL);
        return result;
}

static void u_log_finish(void *data)
{
        u_log_call_t *call = data;

        free(call->allocated);
        pthread_setspecific(u_log_key, NULL);
        pthread_mutex_lock(&u_log_dispatch_lock);
        u_log_in_flight--;
        if (u_log_in_flight == 0) {
                pthread_cond_broadcast(&u_log_drained);
        }
        pthread_mutex_unlock(&u_log_dispatch_lock);
}

static void u_log_format(u_log_call_t *call, ulfius_log_level_t level,
        const char *format, va_list arguments)
{
        char buffer[512];
        const char *message = buffer;
        va_list copy;
        int length;
        int written;

        va_copy(copy, arguments);
        length = vsnprintf(buffer, sizeof(buffer), format, copy);
        va_end(copy);
        if (length >= 0 && (size_t)length >= sizeof(buffer)) {
                if ((uintmax_t)length < (uintmax_t)SIZE_MAX) {
                        call->allocated = malloc((size_t)length + 1);
                }
                if (call->allocated != NULL) {
                        va_copy(copy, arguments);
                        written = vsnprintf(call->allocated, (size_t)length + 1, format, copy);
                        va_end(copy);
                        if (written != length) {
                                length = -1;
                        }
                        message = call->allocated;
                } else {
                        length = -1;
                }
        }
        if (length < 0) {
                message = "Ulfius: unable to format log message";
                level = ULFIUS_LOG_ERROR;
        }
        call->hook.callback(call->hook.user_data, level, message);
}

static void u_log_deliver(u_log_call_t *call, ulfius_log_level_t level,
        const char *format, va_list arguments)
{
        pthread_cleanup_push(u_log_finish, call);
        u_log_format(call, level, format, arguments);
        pthread_cleanup_pop(1);
}

void ulfius_log_message(ulfius_log_level_t level, const char *format, ...)
{
        u_log_call_t call = { { ULFIUS_LOG_NONE, NULL, NULL }, NULL };
        va_list arguments;

        if (level < ULFIUS_LOG_ERROR || level > ULFIUS_LOG_DEBUG || format == NULL ||
                !u_log_prepare_key() || pthread_getspecific(u_log_key) != NULL) {
                return;
        }
        if (pthread_setspecific(u_log_key, &u_log_active_marker) != 0) {
                return;
        }
        if (pthread_mutex_lock(&u_log_dispatch_lock) != 0) {
                pthread_setspecific(u_log_key, NULL);
                return;
        }
        if (u_log_hook.callback == NULL || level > u_log_hook.level ||
                u_log_in_flight == SIZE_MAX) {
                pthread_mutex_unlock(&u_log_dispatch_lock);
                pthread_setspecific(u_log_key, NULL);
                return;
        }
        call.hook = u_log_hook;
        u_log_in_flight++;
        pthread_mutex_unlock(&u_log_dispatch_lock);
        va_start(arguments, format);
        u_log_deliver(&call, level, format, arguments);
        va_end(arguments);
}
