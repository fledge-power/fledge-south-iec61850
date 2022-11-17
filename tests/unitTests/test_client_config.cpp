#include <string>

#include <gtest/gtest.h>

#include <rapidjson/document.h>

// Fledge headers
#include <config_category.h>
#include <plugin_api.h>

#include "plugin.h"
#include "iec61850_client_config.h"

#include "../common/configuration_examples.h"

TEST(IEC61850ClientConfigTest, importDefaultConfig)
{
    ConfigCategory config("TestDefaultConfig", default_config);
    config.setItemsValueFromDefault();

    IEC61850ClientConfig clientConfig;
    clientConfig.importConfig(config);

    ASSERT_EQ(clientConfig.logMinLevel, "info");
    ASSERT_EQ(clientConfig.assetName, "iec61850");
    ASSERT_EQ(clientConfig.iedName, "simpleIO");

    ASSERT_EQ(clientConfig.serverConfigDict.size(), 2);

    ASSERT_EQ(clientConfig.serverConfigDict["0.0.0.0_102"].ipAddress, "0.0.0.0");
    ASSERT_EQ(clientConfig.serverConfigDict["0.0.0.0_102"].mmsPort, 102);

    ASSERT_EQ(clientConfig.serverConfigDict["0.0.0.0_8102"].ipAddress, "0.0.0.0");
    ASSERT_EQ(clientConfig.serverConfigDict["0.0.0.0_8102"].mmsPort, 8102);
}

TEST(IEC61850ClientConfigTest, importConfigWithoutLogLevel)
{
    ConfigCategory config("Config", configWithoutLogLevel);

    IEC61850ClientConfig clientConfig;
    clientConfig.importConfig(config);

    ASSERT_EQ(clientConfig.logMinLevel, "info");
}

TEST(IEC61850ClientConfigTest, importConfigMissingProtocolStack)
{
    ConfigCategory config("Config", configMissingProtocolStack);

    IEC61850ClientConfig clientConfig;

    try {
        clientConfig.importConfig(config);
        FAIL();
    }
    catch (ConfigurationException e) {
        ASSERT_STREQ(e.what(), "Configuration exception: 'Protocol stack' not found");
    }
    catch (...) {
        FAIL();
    }
}

TEST(IEC61850ClientConfigTest, importConfigProtocolStackWithParsingError)
{
    ConfigCategory config("Config", configProtocolStackWithParsingError);

    IEC61850ClientConfig clientConfig;

    try {
        clientConfig.importConfig(config);
        FAIL();
    }
    catch (ConfigurationException e) {
        ASSERT_STREQ(e.what(), "Configuration exception: 'Protocol stack' parsing error");
    }
    catch (...) {
        FAIL();
    }
}

TEST(IEC61850ClientConfigTest, importConfigProtocolStackEmptyConf)
{
    ConfigCategory config("Config", configProtocolStackEmptyConf);

    IEC61850ClientConfig clientConfig;

    try {
        clientConfig.importConfig(config);
        FAIL();
    }
    catch (ConfigurationException e) {
        ASSERT_STREQ(e.what(), "Configuration exception: 'Protocol stack' empty conf");
    }
    catch (...) {
        FAIL();
    }
}

TEST(IEC61850ClientConfigTest, importConfigProtocolStackMissingTransport)
{
    ConfigCategory config("Config", configProtocolStackMissingTransport);

    IEC61850ClientConfig clientConfig;

    try {
        clientConfig.importConfig(config);
        FAIL();
    }
    catch (ConfigurationException e) {
        ASSERT_STREQ(e.what(), "Configuration exception: 'transport layer' configuration is missing");
    }
    catch (...) {
        FAIL();
    }
}

TEST(IEC61850ClientConfigTest, importConfigProtocolStackMissingApplication)
{
    ConfigCategory config("Config", configProtocolStackMissingApplication);

    IEC61850ClientConfig clientConfig;

    try {
        clientConfig.importConfig(config);
        FAIL();
    }
    catch (ConfigurationException e) {
        ASSERT_STREQ(e.what(), "Configuration exception: 'application layer' configuration is missing");
    }
    catch (...) {
        FAIL();
    }
}

TEST(IEC61850ClientConfigTest, importConfigProtocolStackMissingIedName)
{
    ConfigCategory config("Config", configProtocolStackMissingIedName);

    IEC61850ClientConfig clientConfig;

    try {
        clientConfig.importConfig(config);
        FAIL();
    }
    catch (ConfigurationException e) {
        ASSERT_STREQ(e.what(), "Configuration exception: the mandatory 'ied_name' not found");
    }
    catch (...) {
        FAIL();
    }
}

TEST(IEC61850ClientConfigTest, importConfigProtocolStackIedNameBadFormat)
{
    ConfigCategory config("Config", configProtocolStackIedNameBadFormat);

    IEC61850ClientConfig clientConfig;

    try {
        clientConfig.importConfig(config);
        FAIL();
    }
    catch (ConfigurationException e) {
        ASSERT_STREQ(e.what(), "Configuration exception: bad format for the mandatory 'ied_name'");
    }
    catch (...) {
        FAIL();
    }
}

TEST(IEC61850ClientConfigTest, importConfigProtocolStackMissingConnection)
{
    ConfigCategory config("Config", configProtocolStackMissingConnection);

    IEC61850ClientConfig clientConfig;

    try {
        clientConfig.importConfig(config);
        FAIL();
    }
    catch (ConfigurationException e) {
        ASSERT_STREQ(e.what(), "Configuration exception: 'Transport Layer' parsing error: no 'connections'");
    }
    catch (...) {
        FAIL();
    }
}

TEST(IEC61850ClientConfigTest, importConfigProtocolStackConnectionBadFormat)
{
    ConfigCategory config("Config", configProtocolStackConnectionBadFormat);

    IEC61850ClientConfig clientConfig;

    try {
        clientConfig.importConfig(config);
        FAIL();
    }
    catch (ConfigurationException e) {
        ASSERT_STREQ(e.what(), "Configuration exception: 'connections' is not an array -> fail to parse 'Transport Layer'");
    }
    catch (...) {
        FAIL();
    }
}

TEST(IEC61850ClientConfigTest, importConfigProtocolStackMissingSrvIp)
{
    ConfigCategory config("Config", configProtocolStackMissingSrvIp);

    IEC61850ClientConfig clientConfig;

    try {
        clientConfig.importConfig(config);
        FAIL();
    }
    catch (ConfigurationException e) {
        ASSERT_STREQ(e.what(), "Configuration exception: the mandatory 'srv_ip' not found");
    }
    catch (...) {
        FAIL();
    }
}

TEST(IEC61850ClientConfigTest, importConfigProtocolStackgSrvIpBadFormat)
{
    ConfigCategory config("Config", configProtocolStackSrvIpBadFormat);

    IEC61850ClientConfig clientConfig;

    try {
        clientConfig.importConfig(config);
        FAIL();
    }
    catch (ConfigurationException e) {
        ASSERT_STREQ(e.what(), "Configuration exception: bad format for the mandatory 'srv_ip'");
    }
    catch (...) {
        FAIL();
    }
}

TEST(IEC61850ClientConfigTest, importConfigProtocolStackMissingPort)
{
    ConfigCategory config("Config", configProtocolStackMissingPort);

    IEC61850ClientConfig clientConfig;

    try {
        clientConfig.importConfig(config);
        FAIL();
    }
    catch (ConfigurationException e) {
        ASSERT_STREQ(e.what(), "Configuration exception: the mandatory 'port' not found");
    }
    catch (...) {
        FAIL();
    }
}

TEST(IEC61850ClientConfigTest, importConfigProtocolStackPortBadFormat)
{
    ConfigCategory config("Config", configProtocolStackPortBadFormat);

    IEC61850ClientConfig clientConfig;

    try {
        clientConfig.importConfig(config);
        FAIL();
    }
    catch (ConfigurationException e) {
        ASSERT_STREQ(e.what(), "Configuration exception: bad format for the mandatory 'port'");
    }
    catch (...) {
        FAIL();
    }
}

TEST(IEC61850ClientConfigTest, importConfigProtocolStackInvalidIPAddress)
{
    ConfigCategory config("Config", configProtocolStackInvalidIPAddress);

    IEC61850ClientConfig clientConfig;

    try {
        clientConfig.importConfig(config);
        FAIL();
    }
    catch (ConfigurationException e) {
        ASSERT_STREQ(e.what(), "Configuration exception: not a valid IP address for the mandatory 'srv_ip'");
    }
    catch (...) {
        FAIL();
    }
}
