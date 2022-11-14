#include <string>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <rapidjson/document.h>

// Fledge headers
#include <config_category.h>
#include <plugin_api.h>

#include "plugin.h"
#include "iec61850.h"

using namespace ::testing;

using INGEST_CB = void (*)(void *, Reading);

extern "C"
{
    PLUGIN_INFORMATION *plugin_info();
    PLUGIN_HANDLE plugin_init(ConfigCategory *config);
    void plugin_register_ingest(PLUGIN_HANDLE handle, INGEST_CB ingestCallback,
                                void *data);
    void plugin_reconfigure(PLUGIN_HANDLE handle, std::string &newConfig);
    void plugin_shutdown(PLUGIN_HANDLE handle);
    void plugin_start(PLUGIN_HANDLE handle);
};

TEST(IEC61850PluginTest, PluginInfo)
{
    PLUGIN_INFORMATION *info = plugin_info();
    ASSERT_STREQ(info->name, "iec61850");
    ASSERT_STREQ(info->type, PLUGIN_TYPE_SOUTH);
}

TEST(IEC61850PluginTest, PluginInfoConfig)
{
    PLUGIN_INFORMATION *info = plugin_info();
    rapidjson::Document doc;
    doc.Parse(info->config);
    ASSERT_EQ(doc.HasParseError(), false);
    ASSERT_EQ(doc.IsObject(), true);
    ASSERT_EQ(doc.HasMember("plugin"), true);
}

TEST(IEC61850PluginTest, PluginInit)
{
    ConfigCategory config("TestDefaultConfig", default_config);
    config.setItemsValueFromDefault();

    PLUGIN_HANDLE handle = nullptr;

    ASSERT_THAT(handle, IsNull());
    handle = plugin_init(&config);
    ASSERT_THAT(handle, NotNull());

    delete static_cast<IEC61850*>(handle);
}

TEST(IEC61850PluginTest, startNull)
{
    PLUGIN_HANDLE handle = nullptr;

    ASSERT_THAT(handle, IsNull());
    ASSERT_NO_THROW(plugin_start(handle));
}

TEST(IEC61850PluginTest, start)
{
    PLUGIN_HANDLE handle = nullptr;
    ASSERT_THAT(handle, IsNull());

    handle = plugin_init(nullptr);
    ASSERT_THAT(handle, NotNull());

    ASSERT_NO_THROW(plugin_start(handle));

    delete static_cast<IEC61850*>(handle);
}

void ingestCallback(void *data, Reading reading) {}

TEST(IEC61850PluginTest, PluginRegisterIngestWithNullHandle)
{
    ASSERT_THROW(
        plugin_register_ingest(nullptr, ingestCallback, nullptr), std::exception);
}

TEST(IEC61850PluginTest, PluginRegisterIngest)
{
    PLUGIN_HANDLE handle = plugin_init(nullptr);

    ASSERT_NO_THROW(
        plugin_register_ingest(handle, ingestCallback, NULL));

    delete static_cast<IEC61850*>(handle);
}

TEST(IEC61850, PluginShutdown)
{
    PLUGIN_HANDLE handle = plugin_init(nullptr);
    ASSERT_THAT(handle, NotNull());

    ASSERT_NO_THROW(plugin_shutdown(handle));
}

TEST(IEC61850, PluginReconfigure)
{
    std::string new_config(default_config);

    PLUGIN_HANDLE handle = plugin_init(nullptr);
    ASSERT_THAT(handle, NotNull());
    ASSERT_NO_THROW(plugin_reconfigure(handle, new_config));

    delete static_cast<IEC61850*>(handle);
}
