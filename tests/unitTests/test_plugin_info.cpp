#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <rapidjson/document.h>

// Fledge headers
#include <config_category.h>
#include <plugin_api.h>

#include "iec61850.h"

extern "C"
{
    PLUGIN_INFORMATION *plugin_info();
};

TEST(IEC61850PluginInfo, PluginInfo)
{
    PLUGIN_INFORMATION *info = plugin_info();
    ASSERT_STREQ(info->name, "iec61850");
    ASSERT_STREQ(info->type, PLUGIN_TYPE_SOUTH);
}

TEST(IEC61850PluginInfo, PluginInfoConfigParse)
{
    PLUGIN_INFORMATION *info = plugin_info();
    rapidjson::Document doc;
    doc.Parse(info->config);
    ASSERT_EQ(doc.HasParseError(), false);
    ASSERT_EQ(doc.IsObject(), true);
    ASSERT_EQ(doc.HasMember("plugin"), true);
}
