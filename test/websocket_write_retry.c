/* exercise repeated TLS retries with a deterministic monotonic clock */
#undef NDEBUG
#include <assert.h>
#include <time.h>
#include <gnutls/gnutls.h>
#include <ulfius.h>

#ifndef U_DISABLE_WEBSOCKET

static ssize_t retry_send(gnutls_session_t session, const void *data, size_t length);
static int retry_direction(gnutls_session_t session);
static int retry_clock(clockid_t clock_id, struct timespec *value);

#define gnutls_record_send retry_send
#define gnutls_record_get_direction retry_direction
#define clock_gettime retry_clock
#include "../src/u_websocket.c"
#undef gnutls_record_send
#undef gnutls_record_get_direction
#undef clock_gettime

static time_t elapsed;
static uint32_t attempts;
static uint32_t progress_interval;
static uint32_t shutdown_attempt;
static int retry_error;
static struct _websocket_manager *active_manager;

static ssize_t retry_send(gnutls_session_t session, const void *data, size_t length)
{
        (void)session;
        (void)data;
        (void)length;
        attempts++;
        if (attempts == shutdown_attempt) {
                active_manager->close_flag = 1;
        }
        if (progress_interval != 0 && attempts % progress_interval == 0) {
                return 1;
        }
        return retry_error;
}

static int retry_direction(gnutls_session_t session)
{
        (void)session;
        return 1;
}

static int retry_clock(clockid_t clock_id, struct timespec *value)
{
        assert(clock_id == CLOCK_MONOTONIC);
        value->tv_sec = elapsed++;
        value->tv_nsec = 0;
        return 0;
}

static void check_retry(int error, uint32_t progress, uint32_t shutdown, int expected)
{
        struct _websocket_manager manager = {0};
        int sockets[2];
        const uint8_t data[] = {1, 2, 3};

        assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
        manager.type = U_WEBSOCKET_CLIENT;
        manager.tls = 1;
        manager.connected = 1;
        manager.tcp_sock = sockets[0];
        active_manager = &manager;
        elapsed = 0;
        attempts = 0;
        progress_interval = progress;
        shutdown_attempt = shutdown;
        retry_error = error;
        assert(ulfius_websocket_send_frame(&manager, data, sizeof(data)) == expected);
        assert(manager.connected == (expected == U_OK));
        if (shutdown != 0) {
                assert(attempts == shutdown);
        } else if (progress != 0) {
                assert(attempts == progress * sizeof(data));
                assert(elapsed > U_WEBSOCKET_WRITE_IDLE_SECONDS);
        } else {
                assert(attempts > 0 && attempts < U_WEBSOCKET_WRITE_IDLE_SECONDS);
        }
        close(sockets[0]);
        close(sockets[1]);
}

int main(void)
{
        check_retry(GNUTLS_E_AGAIN, 0, 0, U_ERROR);
        check_retry(GNUTLS_E_INTERRUPTED, 0, 0, U_ERROR);
        check_retry(GNUTLS_E_AGAIN, 0, 2, U_ERROR);
        check_retry(GNUTLS_E_AGAIN, 4, 0, U_OK);
        return 0;
}

#else
int main(void)
{
        return 0;
}
#endif
