#ifndef ENGINE_H
#define ENGINE_H

#include <stddef.h>
#include <netinet/in.h>

#include "strutil.h"

struct engine {
    bool running;
    struct sockaddr_in local_ip;
    strutil_t host;
};

int engine_init();
int engine_start();
int engine_stop();

struct engine engine_status(void);

#endif /* ENGINE_H */