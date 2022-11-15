#include <gtest/gtest.h>
#include <gmock/gmock.h>

// South_IEC61850_Plugin headers
#include "iec61850_client_config.h"
#include "iec61850_client_connection.h"

// test utilities headers
#include "../common/mms_server_basic_io/mms_server_basic_io.h"

using namespace ::testing;

TEST(IEC61850ClientConnectionTest, openConnection)
{
    // Test Init
    ServerConnectionParameters connParam;
    connParam.ipAddress = "127.0.0.1";
    connParam.mmsPort = 8102;

    MmsServerBasicIO mmsServer(8102);
    mmsServer.start();

    // Test Body
    IEC61850ClientConnection conn(connParam);

    ASSERT_EQ("127.0.0.1", conn.m_connectionParam.ipAddress);
    ASSERT_EQ(8102, conn.m_connectionParam.mmsPort);

    ASSERT_THAT(conn.m_iedConnection, NotNull());
    ASSERT_EQ(true, conn.isConnected());
    ASSERT_EQ(true, conn.isNoError());
    conn.logError();

    // Test Teardown
    mmsServer.stop();
}
