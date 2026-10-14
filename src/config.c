#include <stdio.h>

#include <wups.h>
#include <wups/config/WUPSConfigItemStub.h>

#include "engine.h"
#include "logger.h"
#include "strutil.h"

WUPSConfigAPICallbackStatus
ConfigMenuOpenedCallback(WUPSConfigCategoryHandle root) {
    struct engine status = engine_status();
    char info_buf[64];
    snprintf(info_buf, sizeof(info_buf), "%.*s", status.host.length,
             status.host.str);

    if (WUPSConfigItemStub_AddToCategory(root, info_buf) !=
        WUPSCONFIG_API_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to add info item to root category");
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
