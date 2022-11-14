#include <string>

#include <gtest/gtest.h>

#include <rapidjson/document.h>

// Fledge headers
#include <config_category.h>
#include <plugin_api.h>

#include "plugin.h"
#include "iec61850_client_config.h"


TEST(IEC61850ClientConfigTest, importDefaultConfig)
{
    ConfigCategory config("TestDefaultConfig", default_config);
    config.setItemsValueFromDefault();

    IEC61850ClientConfig clientConfig;
    clientConfig.importConfig(config);

    ASSERT_EQ(clientConfig.logMinLevel, "info");
    ASSERT_EQ(clientConfig.assetName, "iec61850");

    ASSERT_EQ(clientConfig.serverConfigDict.size(), 2);

    ASSERT_EQ(clientConfig.serverConfigDict["simpleIO_0.0.0.0_102"].serverName, "simpleIO");
    ASSERT_EQ(clientConfig.serverConfigDict["simpleIO_0.0.0.0_102"].ipAddress, "0.0.0.0");
    ASSERT_EQ(clientConfig.serverConfigDict["simpleIO_0.0.0.0_102"].mmsPort, 102);

    ASSERT_EQ(clientConfig.serverConfigDict["simpleIO_0.0.0.0_8102"].serverName, "simpleIO");
    ASSERT_EQ(clientConfig.serverConfigDict["simpleIO_0.0.0.0_8102"].ipAddress, "0.0.0.0");
    ASSERT_EQ(clientConfig.serverConfigDict["simpleIO_0.0.0.0_8102"].mmsPort, 8102);
}
