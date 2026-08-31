#include <wups.h>

#include "engine.h"
#include "logger.h"

/**
    Mandatory plugin information.
    If not set correctly, the loader will refuse to use the plugin.
**/
WUPS_PLUGIN_NAME(APP_NAME);
WUPS_PLUGIN_DESCRIPTION("Description");
WUPS_PLUGIN_VERSION("v0.1");
WUPS_PLUGIN_AUTHOR("fengb");
WUPS_PLUGIN_LICENSE("MIT");

/**
    All of this defines can be used in ANY file.
    It's possible to split it up into multiple files.

**/

WUPS_USE_WUT_DEVOPTAB();    // Use the wut devoptabs
WUPS_USE_STORAGE(APP_NAME); // Unique id for the storage api

/**
    Gets called ONCE when the plugin was loaded.
**/
INITIALIZE_PLUGIN() {
    initLogging();
    engine_init();
    deinitLogging();
}

/**
    Gets called when the plugin will be unloaded.
**/
DEINITIALIZE_PLUGIN() {
    engine_stop();
}

/**
    Gets called when an application starts.
**/
ON_APPLICATION_START() {
    initLogging();
    engine_start();
}

/**
    Gets called when an application request to exit.
**/
ON_APPLICATION_REQUESTS_EXIT() {
    engine_stop();
    deinitLogging();
}