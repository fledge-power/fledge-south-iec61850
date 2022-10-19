#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Fledge headers
#include <config_category.h>
#include <plugin_api.h>

#include "iec61850.h"

using INGEST_CB = void (*)(void *, Reading);

extern "C"
{
    PLUGIN_HANDLE plugin_init(ConfigCategory *config);
    void plugin_register_ingest(PLUGIN_HANDLE *handle, INGEST_CB ingestCallback,
                                void *data);
    Reading plugin_poll(PLUGIN_HANDLE *handle);
    void plugin_reconfigure(PLUGIN_HANDLE *handle, std::string &newConfig);
    bool plugin_write(PLUGIN_HANDLE *handle, std::string &name, std::string &value);
    bool plugin_operation(PLUGIN_HANDLE *handle, std::string &operation, int count,
                          PLUGIN_PARAMETER **params);
    void plugin_shutdown(PLUGIN_HANDLE *handle);
    void plugin_start(PLUGIN_HANDLE *handle);
};

TEST(IEC61850, PluginInit)
{
}
