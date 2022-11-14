#include <string>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Fledge headers
#include <config_category.h>

#include "iec61850_client.h"
#include "iec61850_client_config.h"
#include "mock_iec61850_client_connection.h"

using namespace ::testing;

class IEC61850ClientTest_injectMockConnection_Test
{
    public:
        static void injectMockConnection(IEC61850Client &client,
                                         IEC61850ClientConnectionInterface *mockConnection) {
            while (client.m_connection) {
                // wait the previous 'connection' object is destroyed
            }
            client.m_connection = std::unique_ptr<IEC61850ClientConnectionInterface>(mockConnection);
        }
};


TEST(IEC61850ClientTest, createOneConnection)
{
    ServerConnectionParameters connParam;
    ExchangedData exchangedData;

    IEC61850Client client(nullptr,
                          connParam,
                          exchangedData);

    client.createConnection();
    ASSERT_THAT(client.m_connection, NotNull());
}


TEST(IEC61850ClientTest, reuseCreatedConnection)
{
    ServerConnectionParameters connParam;
    ExchangedData exchangedData;

    IEC61850Client client(nullptr,
                          connParam,
                          exchangedData);

    // Create connection
    client.createConnection();
    IEC61850ClientConnectionInterface *connection_pointer = client.m_connection.get();

    // Reuse the already created connection
    client.createConnection();
    ASSERT_EQ(client.m_connection.get(), connection_pointer);
}

TEST(IEC61850ClientTest, destroyConnection)
{
    ServerConnectionParameters connParam;
    ExchangedData exchangedData;

    IEC61850Client client(nullptr,
                          connParam,
                          exchangedData);

    client.createConnection();
    client.destroyConnection();

    ASSERT_THAT(client.m_connection, IsNull());
}

TEST(IEC61850ClientTest, destroyNullConnection)
{
    ServerConnectionParameters connParam;
    ExchangedData exchangedData;

    IEC61850Client client(nullptr,
                          connParam,
                          exchangedData);

    client.createConnection();
    client.destroyConnection();
    ASSERT_NO_THROW(client.destroyConnection());

    ASSERT_THAT(client.m_connection, IsNull());
}

TEST(IEC61850ClientTest, initializeConnectionInOneTry)
{
    // Configuration of the Mock objects
    MockIEC61850ClientConnection mockConnectedConnection;

    EXPECT_CALL(mockConnectedConnection, isConnected())
        .Times(2)
        .WillRepeatedly(Return(true));
    // End of configuration of the Mock objects

    // Test Init
    ServerConnectionParameters connParam;
    ExchangedData exchangedData;

    IEC61850Client client(nullptr,
                          connParam,
                          exchangedData);
    client.createConnection(); // create a Foo connection before MockConnection injection

    // Test Body
    std::thread initializeConnectionThread(&IEC61850Client::initializeConnection,
                                           &client);

    IEC61850ClientTest_injectMockConnection_Test::injectMockConnection(client, &mockConnectedConnection);

    ASSERT_THAT(client.m_connection, NotNull());
    ASSERT_EQ(true, client.m_connection->isConnected());

    // Test teardown
    initializeConnectionThread.join();

    // Do not delete the MockConnection object, through the unique_ptr
    client.m_connection.release();
}

TEST(IEC61850ClientTest, initializeConnectionFailed)
{
    // Test Init
    ServerConnectionParameters connParam;
    ExchangedData exchangedData;

    IEC61850Client client(nullptr,
                          connParam,
                          exchangedData);

    // Test Body
    ASSERT_THAT(client.m_connection, IsNull());

    std::thread initializeConnectionThread(&IEC61850Client::initializeConnection,
                                           &client);
    sleep(3);
    client.m_stopOrder = true;

    ASSERT_THAT(client.m_connection, NotNull());
    ASSERT_EQ(false, client.m_connection->isConnected());

    // Test teardown
    initializeConnectionThread.join();
}

TEST(IEC61850ClientTest, startAndStop)
{
    // Configuration of the Mock objects
    auto empty_mms = std::make_shared<WrappedMms>();
    empty_mms->setMmsValue(nullptr);

    MockIEC61850ClientConnection mockConnectedConnection;

    EXPECT_CALL(mockConnectedConnection, isConnected())
        .Times(4)
        .WillRepeatedly(Return(true));

    EXPECT_CALL(mockConnectedConnection, readMms(_, _))
        .Times(2)
        .WillRepeatedly(Return(empty_mms));

    EXPECT_CALL(mockConnectedConnection, isNoError())
        .WillRepeatedly(Return(true));
    // End of configuration of the Mock objects

    // Test Init
    ServerConnectionParameters connParam;
    ExchangedData exchangedData;

    IEC61850Client client(nullptr,
                          connParam,
                          exchangedData);

    // Test Body
    ASSERT_THAT(client.m_connection, IsNull());
    ASSERT_EQ(false, client.m_stopOrder);
    ASSERT_EQ(false, client.m_backgroundLaunchThread.joinable());

    client.createConnection(); // create a Foo connection before MockConnection injection

    std::thread startClientThread(&IEC61850Client::start,
                                  &client);

    IEC61850ClientTest_injectMockConnection_Test::injectMockConnection(client, &mockConnectedConnection);

    sleep(2);

    ASSERT_THAT(client.m_connection, NotNull());
    ASSERT_EQ(true, client.m_connection->isConnected());
    ASSERT_EQ(false, client.m_stopOrder);

    // Do not delete the MockConnection object, through the unique_ptr
    client.m_connection.release();

    client.stop();
    ASSERT_EQ(true, client.m_stopOrder);
    ASSERT_THAT(client.m_connection, IsNull());

    // Test teardown
    startClientThread.join();
}
