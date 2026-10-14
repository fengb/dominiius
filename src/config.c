#include <stdio.h>

#include <wups.h>
#include <wups/config/WUPSConfigItemStub.h>

#include "engine.h"
#include "logger.h"
#include "strutil.h"

WUPSConfigAPICallbackStatus
ConfigMenuOpenedCallback(WUPSConfigCategoryHandle root) {
    strutil_t machine_name = engine_get_machine_name();
    char status_buf[64];
    snprintf(status_buf, sizeof(status_buf), "%.*s", machine_name.length,
             machine_name.str);

    if (WUPSConfigItemStub_AddToCategory(root, status_buf) !=
        WUPSCONFIG_API_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to add status item to root category");
        return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;
    }

    return WUPSCONFIG_API_CALLBACK_RESULT_SUCCESS;
}

void ConfigMenuClosedCallback() {}

void config_init() {
    WUPSConfigAPIOptionsV1 configOptions = {.name = APP_NAME};
    if (WUPSConfigAPI_Init(configOptions, ConfigMenuOpenedCallback,
                           ConfigMenuClosedCallback) !=
        WUPSCONFIG_API_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to init config api");
    }
}
