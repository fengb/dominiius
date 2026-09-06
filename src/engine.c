#include <coreinit/event.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <errno.h>
#include <nn/ac.h>

#include "hostname.h"
#include "logger.h"
#include "mdns_ipv4_shim.h"

#define LIVENESS_CHECK OSSecondsToTicks(5 * 60)

static OSThread s_engine_thread;
static uint8_t s_engine_thread_stack[16384]; // 16KB stack (adjust as needed)

static bool s_engine_running = false;
static OSEvent s_stop_event;

static struct sockaddr_in s_local_ip;

static char s_name_buf[20];
static mdns_string_t s_machine_name = {
    .str = s_name_buf,
    .length = 0,
};

static int query_callback(int sock, const struct sockaddr *from, size_t addrlen,
                          mdns_entry_type_t entry, uint16_t query_id,
                          uint16_t rtype, uint16_t rclass, uint32_t ttl,
                          const void *data, size_t size, size_t name_offset,
                          size_t name_length, size_t record_offset,
                          size_t record_length, void *user_data) {
    // We only care about incoming questions asking for A records
    if (entry != MDNS_ENTRYTYPE_QUESTION || rtype != MDNS_RECORDTYPE_A) {
        return 0;
    }

    char name_buf[256];
    mdns_string_t queried_name = mdns_string_extract(
        data, size, &name_offset, name_buf, sizeof(name_buf));

    if (queried_name.length != s_machine_name.length ||
        strncasecmp(queried_name.str, s_machine_name.str,
                    s_machine_name.length) != 0) {
        return 0;
    }

    mdns_record_t answer = {.name = queried_name,
                            .type = MDNS_RECORDTYPE_A,
                            .rclass = 0,
                            .ttl = 120,
                            .data = {.a = {.addr = s_local_ip}}};

    char answer_buf[256];
    if (rclass & MDNS_UNICAST_RESPONSE) {
        DEBUG_FUNCTION_LINE_INFO(
            "%.*s => %s", queried_name.length, queried_name.str,
            inet_ntoa(((struct sockaddr_in *)from)->sin_addr));
        mdns_query_answer_unicast(sock, from, addrlen, answer_buf,
                                  sizeof(answer_buf), query_id, rtype,
                                  queried_name.str, queried_name.length, answer,
                                  NULL, 0, NULL, 0);
    } else {
        DEBUG_FUNCTION_LINE_INFO("%.*s => multicast", queried_name.length,
                                 queried_name.str);
        mdns_query_answer_multicast(sock, answer_buf, sizeof(answer_buf),
                                    answer, NULL, 0, NULL, 0);
    }

    return 0;
}

static int engine_connect() {
    if (!NNResult_IsSuccess(ACInitialize())) {
        return -1;
    }

    uint32_t ip_address;
    if (!NNResult_IsSuccess(ACGetAssignedAddress(&ip_address))) {
        return -2;
    }

    if (ip_address == 0) {
        DEBUG_FUNCTION_LINE_INFO("No local address (0.0.0.0)");
        return -3;
    }

    s_local_ip.sin_family = AF_INET;
    s_local_ip.sin_port = htons(MDNS_PORT);
    s_local_ip.sin_addr.s_addr = ip_address;

    DEBUG_FUNCTION_LINE_INFO("Address: %s:%d", inet_ntoa(s_local_ip.sin_addr),
                             MDNS_PORT);

    int sock = mdns_socket_open_ipv4(&s_local_ip);
    if (sock < 0) {
        DEBUG_FUNCTION_LINE_ERR("Failed to open IPv4 mDNS socket");
        return -4;
    }
    DEBUG_FUNCTION_LINE_INFO("Socket: %d", sock);

    return sock;
}

static int engine_serve(int sock) {
    OSTime last_mdns = OSGetSystemTime();
    while (s_engine_running) {
        char recv_buf[1024]; // Max supported size is tested to be 1478

        // mdns_socket_listen handles recvfrom and triggers the callback
        size_t records = mdns_socket_listen(sock, recv_buf, sizeof(recv_buf),
                                            query_callback, NULL);
        if (records > 0) {
            last_mdns = OSGetSystemTime();
            continue;
        }

        switch (errno) {
            // case EAGAIN: // dupe of EWOULDBLOCK within Wii U
        case EWOULDBLOCK: // Socket has no data
            if (OSGetSystemTime() - last_mdns > LIVENESS_CHECK) {
                // mDNS/UDP might not surface a dead network to a listening
                // socket, so treat prolonged silence as disconnected.
                return -2;
            }
            OSWaitEventWithTimeout(&s_stop_event, OSMillisecondsToTicks(50));
            continue;
        default:
            DEBUG_FUNCTION_LINE_ERR("Listen errno: %d", errno);
            // Handle socket error or shutdown
            return -1;
        }
    }

    return 0;
}

static int engine_main(int argc, const char **argv) {
    s_engine_running = true;
    unsigned int backoff = 0;

    while (s_engine_running) {
        int sock = engine_connect();
        if (sock < 0) {
            OSWaitEventWithTimeout(&s_stop_event,
                                   OSSecondsToTicks(1ULL << backoff));
            backoff += (backoff < 6);
            continue;
        }

        int res = engine_serve(sock);
        mdns_socket_close(sock);
        backoff = 0;
        OSWaitEventWithTimeout(&s_stop_event, OSSecondsToTicks(1ULL));
    }

    s_engine_running = false;
    return 0;
}

int engine_init() {
    OSInitEvent(&s_stop_event, FALSE, OS_EVENT_MODE_MANUAL);

    ssize_t len = hostname_load(s_name_buf, sizeof(s_name_buf));
    if (len <= 0) {
        DEBUG_FUNCTION_LINE_ERR("Cannot load hostname?");
        return -1;
    }
    s_machine_name.length = len;
    DEBUG_FUNCTION_LINE_INFO("Hostname: %.*s", len, s_machine_name.str);

    return 0;
}

int engine_start() {
    OSResetEvent(&s_stop_event);

    bool success = OSCreateThread(
        &s_engine_thread,                                      // Thread object
        engine_main,                                           // Entry function
        0,                                                     // argc
        NULL,                                                  // argv
        s_engine_thread_stack + sizeof(s_engine_thread_stack), // Stack top
        sizeof(s_engine_thread_stack),                         // Stack size
        16, // Priority (lower number = higher priority, 16 is safe)
        OS_THREAD_ATTRIB_DETACHED // Attributes
    );

#ifdef DEBUG
    OSSetThreadStackUsage(&s_engine_thread);
#endif

    if (success) {
        OSResumeThread(&s_engine_thread);
    }

    return 0;
}

int engine_stop() {
    s_engine_running = false;
    OSSignalEvent(&s_stop_event);

#ifdef DEBUG
    DEBUG_FUNCTION_LINE_INFO("Max thread use: %d",
                             OSCheckThreadStackUsage(&s_engine_thread));
#endif

    return 0;
}