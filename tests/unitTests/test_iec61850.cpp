#include <string>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Fledge headers
#include <config_category.h>
#include <plugin_api.h>

#include "plugin.h"
#include "iec61850.h"

using namespace ::testing;

unsigned int global_ingestCallback_count = 0;

TEST(IEC61850Test, createObjectWithEmptyConfig)
{
    IEC61850 iec61850;

    ASSERT_THAT(iec61850.m_config, NotNull());
    ASSERT_EQ(0, iec61850.m_config->serverConfigDict.size());
}

TEST(IEC61850Test, setValidConfig)
{
    ConfigCategory config("TestDefaultConfig", default_config);
    config.setItemsValueFromDefault();

    IEC61850 iec61850;

    iec61850.setConfig(config);

    ASSERT_THAT(iec61850.m_config, NotNull());
    ASSERT_EQ(2, iec61850.m_config->serverConfigDict.size());
}

TEST(IEC61850Test, getDefaultLogLevel)
{
    ConfigCategory config("TestDefaultConfig", default_config);
    config.setItemsValueFromDefault();

    IEC61850 iec61850;

    iec61850.setConfig(config);

    ASSERT_EQ("info", iec61850.getLogMinLevel());
}

TEST(IEC61850Test, getCustomizedLogLevel)
{
    ConfigCategory config("TestDefaultConfig", default_config);
    config.setItemsValueFromDefault();

    IEC61850 iec61850;

    iec61850.setConfig(config);
    iec61850.m_config->logMinLevel = "debug";

    ASSERT_EQ("debug", iec61850.getLogMinLevel());
}


TEST(IEC61850Test, startClient)
{
    ConfigCategory config("TestDefaultConfig", default_config);
    config.setItemsValueFromDefault();

    IEC61850 iec61850;
    iec61850.setConfig(config);

    ASSERT_EQ(0, iec61850.m_clients.size());
    iec61850.start();

    ASSERT_EQ(2, iec61850.m_clients.size());
}

TEST(IEC61850Test, stopClient)
{
    ConfigCategory config("TestDefaultConfig", default_config);
    config.setItemsValueFromDefault();

    IEC61850 iec61850;
    iec61850.setConfig(config);

    iec61850.start();
    ASSERT_EQ(2, iec61850.m_clients.size());

    sleep(1);

    iec61850.stop();
    ASSERT_EQ(0, iec61850.m_clients.size());
}


void ingestDemoCallback(INGEST_DATA_TYPE, Reading reading)
{
    global_ingestCallback_count++;
}

TEST(IEC61850Test, registerIngestCallback)
{
    DatapointValue datapointValue(0.0);
    Datapoint *datapoint1 = new Datapoint("data_name", datapointValue);
    Datapoint *datapoint2 = new Datapoint("data_name", datapointValue);
    Datapoint *datapoint3 = new Datapoint("data_name", datapointValue);

    std::vector<Datapoint *> points1;
    points1.push_back(datapoint1);

    std::vector<Datapoint *> points2;
    points2.push_back(datapoint2);

    std::vector<Datapoint *> points3;
    points3.push_back(datapoint3);

    IEC61850 iec61850;
    ASSERT_THAT(iec61850.m_data, IsNull());

    int fooIngestDataType = 0;
    INGEST_DATA_TYPE ingestDataType = &fooIngestDataType;

    iec61850.registerIngest(ingestDataType,
                            ingestDemoCallback);

    ASSERT_THAT(iec61850.m_data, NotNull());
    ASSERT_EQ(ingestDataType, iec61850.m_data);

    iec61850.ingest(points1);
    iec61850.ingest(points2);
    iec61850.ingest(points3);

    ASSERT_EQ(global_ingestCallback_count, 3);
}
