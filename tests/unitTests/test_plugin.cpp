#include <config_category.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iec61850.h>
#include <plugin_api.h>

#include <string>

using namespace std;

typedef void (*INGEST_CB)(void *, Reading);

extern "C"
{
    PLUGIN_HANDLE plugin_init(ConfigCategory *config);
    void plugin_register_ingest(PLUGIN_HANDLE *handle, INGEST_CB cb,
                                void *data);
    Reading plugin_poll(PLUGIN_HANDLE *handle);
    void plugin_reconfigure(PLUGIN_HANDLE *handle, string &newConfig);
    bool plugin_write(PLUGIN_HANDLE *handle, string &name, string &value);
    bool plugin_operation(PLUGIN_HANDLE *handle, string &operation, int count,
                          PLUGIN_PARAMETER **params);
    void plugin_shutdown(PLUGIN_HANDLE *handle);
    void plugin_start(PLUGIN_HANDLE *handle);
};

TEST(IEC61850, PluginInit)
{
}
