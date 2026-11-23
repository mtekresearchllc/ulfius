/* exercise public logging callbacks and their lifetime contract */
#undef NDEBUG
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ulfius.h>

typedef struct {
        pthread_mutex_t lock;
        pthread_cond_t ready;
        int entered;
        int calls;
        int recursive;
        int exit_thread;
        int wait;
        char text[8192];
        ulfius_log_level_t level;
} log_probe_t;

static void unlock_probe(void *data)
{
        pthread_mutex_unlock(data);
}

static void capture(void *data, ulfius_log_level_t level, const char *message)
{
        log_probe_t *probe = data;

        assert(probe != NULL);
        pthread_mutex_lock(&probe->lock);
        probe->calls++;
        probe->level = level;
        snprintf(probe->text, sizeof(probe->text), "%s", message);
        probe->entered = 1;
        pthread_cond_broadcast(&probe->ready);
        pthread_cleanup_push(unlock_probe, &probe->lock);
        while (probe->wait) {
                pthread_cond_wait(&probe->ready, &probe->lock);
        }
        pthread_cleanup_pop(1);
        if (probe->recursive) {
                ulfius_log_message(ULFIUS_LOG_ERROR, "recursive");
                assert(ulfius_set_log_callback(ULFIUS_LOG_NONE, NULL, NULL) == U_ERROR);
        }
        if (probe->exit_thread) {
                pthread_exit(NULL);
        }
}

static void *emit_long(void *data)
{
        (void)data;
        ulfius_log_message(ULFIUS_LOG_INFO, "%06000d", 7);
        return NULL;
}

static void basic_and_lifetime(void)
{
        log_probe_t probe = { .lock = PTHREAD_MUTEX_INITIALIZER,
                .ready = PTHREAD_COND_INITIALIZER };
        pthread_t thread;
        void *result;

        ulfius_log_message(ULFIUS_LOG_ERROR, "disabled");
        assert(ulfius_set_log_callback(ULFIUS_LOG_INVALID, capture, &probe) == U_ERROR_PARAMS);
        assert(ulfius_set_log_callback(ULFIUS_LOG_NONE, capture, &probe) == U_ERROR_PARAMS);
        assert(ulfius_set_log_callback(ULFIUS_LOG_NONE, NULL, &probe) == U_ERROR_PARAMS);
        assert(ulfius_set_log_callback(ULFIUS_LOG_ERROR, NULL, NULL) == U_ERROR_PARAMS);
        assert(ulfius_set_log_callback(ULFIUS_LOG_WARNING, capture, &probe) == U_OK);
        ulfius_log_message(ULFIUS_LOG_INFO, "filtered");
        ulfius_log_message(ULFIUS_LOG_NONE, "ignored");
        ulfius_log_message(ULFIUS_LOG_INVALID, "ignored");
        ulfius_log_message(ULFIUS_LOG_ERROR, NULL);
        assert(probe.calls == 0);
        ulfius_log_message(ULFIUS_LOG_WARNING, "value=%d %s", 42, "100%");
        assert(probe.calls == 1 && probe.level == ULFIUS_LOG_WARNING);
        assert(strcmp(probe.text, "value=42 100%") == 0);
        probe.recursive = 1;
        ulfius_log_message(ULFIUS_LOG_ERROR, "outer");
        assert(probe.calls == 2 && strcmp(probe.text, "outer") == 0);
        probe.recursive = 0;
        assert(ulfius_set_log_callback(ULFIUS_LOG_DEBUG, capture, &probe) == U_OK);
        ulfius_log_message(ULFIUS_LOG_INFO, "%06000d", 7);
        assert(strlen(probe.text) == 6000 && probe.text[5999] == '7');
        probe.wait = 1;
        probe.entered = 0;
        assert(pthread_create(&thread, NULL, emit_long, NULL) == 0);
        pthread_mutex_lock(&probe.lock);
        while (!probe.entered) {
                pthread_cond_wait(&probe.ready, &probe.lock);
        }
        pthread_mutex_unlock(&probe.lock);
        assert(pthread_cancel(thread) == 0);
        assert(pthread_join(thread, &result) == 0 && result == PTHREAD_CANCELED);
        assert(ulfius_set_log_callback(ULFIUS_LOG_NONE, NULL, NULL) == U_OK);
        probe.wait = 0;
        probe.exit_thread = 1;
        assert(ulfius_set_log_callback(ULFIUS_LOG_DEBUG, capture, &probe) == U_OK);
        assert(pthread_create(&thread, NULL, emit_long, NULL) == 0);
        assert(pthread_join(thread, NULL) == 0);
        assert(ulfius_set_log_callback(ULFIUS_LOG_NONE, NULL, NULL) == U_OK);
        pthread_cond_destroy(&probe.ready);
        pthread_mutex_destroy(&probe.lock);
}

typedef struct {
        pthread_mutex_t lock;
        int stop;
        size_t calls;
} stress_state_t;

typedef struct {
        stress_state_t *state;
        int magic;
} stress_hook_t;

static void stress_capture(void *data, ulfius_log_level_t level, const char *message)
{
        stress_hook_t *hook = data;

        assert(hook->magic == 12345);
        assert(level == ULFIUS_LOG_INFO && strcmp(message, "stress") == 0);
        pthread_mutex_lock(&hook->state->lock);
        hook->state->calls++;
        pthread_mutex_unlock(&hook->state->lock);
}

static void *stress_emit(void *data)
{
        stress_state_t *state = data;
        int stop;

        do {
                ulfius_log_message(ULFIUS_LOG_INFO, "stress");
                pthread_mutex_lock(&state->lock);
                stop = state->stop;
                pthread_mutex_unlock(&state->lock);
        } while (!stop);
        return NULL;
}

static void concurrent_replacement(void)
{
        stress_state_t state = { .lock = PTHREAD_MUTEX_INITIALIZER };
        pthread_t threads[8];
        stress_hook_t *previous = NULL;
        size_t i;

        for (i = 0; i < 8; i++) {
                assert(pthread_create(&threads[i], NULL, stress_emit, &state) == 0);
        }
        for (i = 0; i < 2000; i++) {
                stress_hook_t *hook = malloc(sizeof(*hook));
                assert(hook != NULL);
                hook->state = &state;
                hook->magic = 12345;
                assert(ulfius_set_log_callback(ULFIUS_LOG_INFO, stress_capture, hook) == U_OK);
                if (previous != NULL) {
                        previous->magic = -1;
                        free(previous);
                }
                previous = hook;
                ulfius_log_message(ULFIUS_LOG_INFO, "stress");
        }
        assert(ulfius_set_log_callback(ULFIUS_LOG_NONE, NULL, NULL) == U_OK);
        previous->magic = -1;
        free(previous);
        pthread_mutex_lock(&state.lock);
        state.stop = 1;
        pthread_mutex_unlock(&state.lock);
        for (i = 0; i < 8; i++) {
                assert(pthread_join(threads[i], NULL) == 0);
        }
        assert(state.calls >= 2000);
        pthread_mutex_destroy(&state.lock);
}

int main(void)
{
        basic_and_lifetime();
        concurrent_replacement();
        puts("logging callbacks, cancellation and concurrent replacement passed");
        return 0;
}
