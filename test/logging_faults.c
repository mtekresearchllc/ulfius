/* inject formatter and TLS failures without changing the public library */
#undef NDEBUG
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ulfius.h>

static int fail_allocation;
static int fail_format;
static int fail_key;
static int fail_specific;
static int calls;

static void *fault_malloc(size_t size)
{
        return fail_allocation ? NULL : malloc(size);
}

static int fault_vsnprintf(char *text, size_t size, const char *format, va_list arguments)
{
        return fail_format ? -1 : vsnprintf(text, size, format, arguments);
}

static int fault_key_create(pthread_key_t *key, void (*destructor)(void *))
{
        return fail_key ? EAGAIN : pthread_key_create(key, destructor);
}

static int fault_setspecific(pthread_key_t key, const void *data)
{
        return fail_specific ? ENOMEM : pthread_setspecific(key, data);
}

#define malloc fault_malloc
#define vsnprintf fault_vsnprintf
#define pthread_key_create fault_key_create
#define pthread_setspecific fault_setspecific
#define ulfius_set_log_callback fault_set_log_callback
#define ulfius_log_message fault_log_message
#include "../src/u_logging.c"
#undef ulfius_log_message
#undef ulfius_set_log_callback
#undef pthread_setspecific
#undef pthread_key_create
#undef vsnprintf
#undef malloc

static void fault_capture(void *data, ulfius_log_level_t level, const char *message)
{
        (void)data;
        calls++;
        assert(level == ULFIUS_LOG_ERROR);
        assert(strcmp(message, "Ulfius: unable to format log message") == 0);
}

int main(void)
{
        pid_t child;
        int status;

        child = fork();
        assert(child >= 0);
        if (child == 0) {
                fail_key = 1;
                assert(fault_set_log_callback(ULFIUS_LOG_DEBUG, fault_capture, NULL) == U_ERROR);
                fault_log_message(ULFIUS_LOG_ERROR, "ignored");
                assert(calls == 0);
                _exit(0);
        }
        assert(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0);
        assert(fault_set_log_callback(ULFIUS_LOG_DEBUG, fault_capture, NULL) == U_OK);
        fail_specific = 1;
        fault_log_message(ULFIUS_LOG_ERROR, "ignored");
        assert(calls == 0);
        fail_specific = 0;
        fail_allocation = 1;
        fault_log_message(ULFIUS_LOG_INFO, "%06000d", 7);
        assert(calls == 1);
        fail_allocation = 0;
        fail_format = 1;
        fault_log_message(ULFIUS_LOG_WARNING, "anything");
        assert(calls == 2);
        assert(fault_set_log_callback(ULFIUS_LOG_NONE, NULL, NULL) == U_OK);
        puts("logging formatter and TLS failures passed");
        return 0;
}
