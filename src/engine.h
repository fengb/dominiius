#ifndef ENGINE_H
#define ENGINE_H

#include <stddef.h>

#include "strutil.h"

int engine_init();
int engine_start();
int engine_stop();

strutil_t engine_get_machine_name(void);

#endif /* ENGINE_H */