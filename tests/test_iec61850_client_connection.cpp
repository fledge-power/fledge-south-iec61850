#include "libiec61850/iec61850_server.h"
#include <config_category.h>
#include <gtest/gtest.h>
#include <iec61850.hpp>
#include <plugin_api.h>
#include <string.h>

#include <boost/thread.hpp>
#include <libiec61850/hal_thread.h>
#include <utility>
#include <vector>

using namespace std;

// PLUGIN DEFAULT PROTOCOL STACK CONF
static string protocol_config = QUOTE ({
    "protocol_stack" : {
        "name" : "iec61850client",
        "version" : "0.0.1",
        "transport_layer" : {
            "ied_name" : "IED1",
            "connections" :
                [ { "ip_addr" : "127.0.0.1", "port" : 10002, "tls" : false } ]
        },
        "application_layer" : { "polling_interval" : 0 }
    }
});

static string protocol_config_1 = QUOTE ({
    "protocol_stack" : {
        "name" : "iec61850client",
        "version" : "0.0.1",
        "transport_layer" : {
            "ied_name" : "IED1",
            "connections" :
                [ { "ip_addr" : "127.0.0.1", "port" : 10002, "tls" : true } ],
            "tls" : false
        },
        "application_layer" : { "polling_interval" : 0 }
    }
});

static string protocol_config_2 = QUOTE ({
    "protocol_stack" : {
        "name" : "iec61850client",
        "version" : "0.0.1",
        "transport_layer" : {
            "ied_name" : "IED1",
            "connections" : [
                { "ip_addr" : "127.0.0.1", "port" : 10002, "tls" : false },
                { "ip_addr" : "127.0.0.1", "port" : 10003, "tls" : false }
            ]
        },
        "application_layer" : { "polling_interval" : 0 }
    }
});

// PLUGIN DEFAULT EXCHANGED DATA CONF

static string exchanged_data
    = QUOTE ({ "exchanged_data" : { "datapoints" : [] } });

// PLUGIN DEFAULT TLS CONF
static string tls_config = QUOTE ({
    "tls_conf" : {
        "private_key" : "server-key.pem",
        "own_cert" : "server.cer",
        "ca_certs" : [ { "cert_file" : "root.cer" } ]
    }
});

static string tls_config_2 = QUOTE ({
    "tls_conf" : {
        "private_key" : "iec61850_client.key",
        "own_cert" : "iec61850_client.cer",
        "ca_certs" : [ { "cert_file" : "iec61850_ca.cer" } ],
        "remote_certs" : [ { "cert_file" : "iec61850_server.cer" } ]
    }
});

class ConnectionHandlingTest : public testing::Test
{
  protected:
    IEC61850* iec61850 = nullptr;
    int ingestCallbackCalled = 0;
    Reading* storedReading = nullptr;
    int clockSyncHandlerCalled = 0;
    std::vector<Reading*> storedReadings;

    int asduHandlerCalled = 0;
    IedConnection lastConnection = nullptr;
    int lastOA = 0;
    int openConnections = 0;
    int activations = 0;
    int deactivations = 0;
    int maxConnections = 0;

    void
    SetUp () override
    {
        iec61850 = new IEC61850 ();

        iec61850->registerIngest (this, ingestCallback);
    }

    void
    TearDown () override
    {
        iec61850->stop ();
        delete iec61850;

        for (auto reading : storedReadings)
        {
            delete reading;
        }
    }

    static bool
    hasChild (Datapoint& dp, std::string childLabel)
    {
        DatapointValue& dpv = dp.getData ();

        auto dps = dpv.getDpVec ();

        for (auto sdp : *dps)
        {
            if (sdp->getName () == childLabel)
            {
                return true;
            }
        }

        return false;
    }

    static Datapoint*
    getChild (Datapoint& dp, std::string childLabel)
    {
        DatapointValue& dpv = dp.getData ();

        auto dps = dpv.getDpVec ();

        for (Datapoint* childDp : *dps)
        {
            if (childDp->getName () == childLabel)
            {
                return childDp;
            }
        }

        return nullptr;
    }

    static int64_t
    getIntValue (Datapoint* dp)
    {
        DatapointValue dpValue = dp->getData ();
        return dpValue.toInt ();
    }

    static std::string
    getStrValue (Datapoint* dp)
    {
        return dp->getData ().toStringValue ();
    }

    static bool
    hasObject (Reading& reading, std::string label)
    {
        std::vector<Datapoint*> dataPoints = reading.getReadingData ();

        for (Datapoint* dp : dataPoints)
        {
            if (dp->getName () == label)
            {
                return true;
            }
        }

        return false;
    }

    static Datapoint*
    getObject (Reading& reading, std::string label)
    {
        std::vector<Datapoint*> dataPoints = reading.getReadingData ();

        for (Datapoint* dp : dataPoints)
        {
            if (dp->getName () == label)
            {
                return dp;
            }
        }

        return nullptr;
    }

    static void
    ingestCallback (void* parameter, Reading reading)
    {
        auto self = (ConnectionHandlingTest*)parameter;

        printf ("ingestCallback called -> asset: (%s)\n",
                reading.getAssetName ().c_str ());

        std::vector<Datapoint*> dataPoints = reading.getReadingData ();

        for (Datapoint* sdp : dataPoints)
        {
            printf ("name: %s value: %s\n", sdp->getName ().c_str (),
                    sdp->getData ().toString ().c_str ());
        }
        self->storedReading = new Reading (reading);

        self->storedReadings.push_back (self->storedReading);

        self->ingestCallbackCalled++;
    }
};

TEST_F (ConnectionHandlingTest, SingleConnection)
{
    iec61850->setJsonConfig (protocol_config, exchanged_data, tls_config);

    IedModel* model = ConfigFileParser_createModelFromConfigFileEx (
        "../tests/data/simpleIO_direct_control.cfg");

    IedServer server = IedServer_create (model);

    IedServer_start (server, 10002);
    iec61850->start ();
    Thread_sleep(1000);

    auto start = std::chrono::high_resolution_clock::now ();
    auto timeout = std::chrono::seconds (10);
    while (!iec61850->m_client->m_active_connection || !iec61850->m_client->m_active_connection->m_connection || IedConnection_getState (
               iec61850->m_client->m_active_connection->m_connection)
           != IED_STATE_CONNECTED)
    {
        auto now = std::chrono::high_resolution_clock::now ();
        if (now - start > timeout)
        {
            IedServer_stop (server);
            IedServer_destroy (server);
            IedModel_destroy (model);
            FAIL () << "Connection not established within timeout";
            break;
        }
        Thread_sleep (10);
    }


    IedServer_stop (server);
    IedServer_destroy (server);
    IedModel_destroy (model);
}

TEST_F (ConnectionHandlingTest, SingleConnectionReconnect)
{
    iec61850->setJsonConfig (protocol_config, exchanged_data, tls_config);

    IedModel* model = ConfigFileParser_createModelFromConfigFileEx (
        "../tests/data/simpleIO_direct_control.cfg");

    IedServer server = IedServer_create (model);

    IedServer_start (server, 10002);
    iec61850->start ();

    Thread_sleep (1000);

    auto start = std::chrono::high_resolution_clock::now ();
    auto timeout = std::chrono::seconds (10);
    while (!iec61850->m_client->m_active_connection || !iec61850->m_client->m_active_connection->m_connection || IedConnection_getState (
               iec61850->m_client->m_active_connection->m_connection)
           != IED_STATE_CONNECTED)
    {
        auto now = std::chrono::high_resolution_clock::now ();
        if (now - start > timeout)
        {
            IedServer_stop (server);
            IedServer_destroy (server);
            IedModel_destroy (model);
            FAIL () << "Connection not established within timeout";
            break;
        }
        Thread_sleep (10);
    }

    IedServer_stop (server);

    Thread_sleep (2000);

    IedServer_start (server, 10002);

    start = std::chrono::high_resolution_clock::now ();
    timeout = std::chrono::seconds (20);
    while (!iec61850->m_client->m_active_connection
           || !iec61850->m_client->m_active_connection->m_connection
           || IedConnection_getState (
                  iec61850->m_client->m_active_connection->m_connection)
                  != IED_STATE_CONNECTED)
    {
        auto now = std::chrono::high_resolution_clock::now ();
        if (now - start > timeout)
        {
            IedServer_stop (server);
            IedServer_destroy (server);
            IedModel_destroy (model);
            FAIL () << "Connection not established within timeout";
            break;
        }
        Thread_sleep (10);
    }

    IedServer_stop (server);
    IedServer_destroy (server);
    IedModel_destroy (model);
}

TEST_F (ConnectionHandlingTest, SingleConnectionTLS)
{
    iec61850->setJsonConfig (protocol_config_1, exchanged_data, tls_config_2);

    IedModel* model = ConfigFileParser_createModelFromConfigFileEx (
        "../tests/data/simpleIO_direct_control.cfg");

    setenv ("FLEDGE_DATA", "../tests/data", 1);

    TLSConfiguration tlsConfig = TLSConfiguration_create ();

    TLSConfiguration_addCACertificateFromFile (
        tlsConfig, "../tests/data/etc/certs/iec61850_ca.cer");
    TLSConfiguration_setOwnCertificateFromFile (
        tlsConfig, "../tests/data/etc/certs/iec61850_server.cer");
    TLSConfiguration_setOwnKeyFromFile (
        tlsConfig, "../tests/data/etc/certs/iec61850_server.key", NULL);
    TLSConfiguration_addAllowedCertificateFromFile (
        tlsConfig, "../tests/data/etc/certs/iec61850_client.cer");
    TLSConfiguration_setChainValidation (tlsConfig, true);
    TLSConfiguration_setAllowOnlyKnownCertificates (tlsConfig, true);

    IedServer server = IedServer_createWithTlsSupport (model, tlsConfig);

    IedServer_start (server, 10002);
    iec61850->start ();

    Thread_sleep(1000);
    
    auto start = std::chrono::high_resolution_clock::now ();
    auto timeout = std::chrono::seconds (10);
    while (!iec61850->m_client->m_active_connection || !iec61850->m_client->m_active_connection->m_connection || IedConnection_getState (
               iec61850->m_client->m_active_connection->m_connection)
           != IED_STATE_CONNECTED)
    {
        auto now = std::chrono::high_resolution_clock::now ();
        if (now - start > timeout)
        {
            IedServer_stop (server);
            IedServer_destroy (server);
            IedModel_destroy (model);
            FAIL () << "Connection not established within timeout";
            break;
        }
        Thread_sleep (10);
    }

    IedServer_stop (server);
    IedServer_destroy (server);
    IedModel_destroy (model);
    TLSConfiguration_destroy (tlsConfig);
}

TEST_F (ConnectionHandlingTest, TwoConnectionsBackup)
{
    iec61850->setJsonConfig (protocol_config_2, exchanged_data, tls_config);

    IedModel* model1 = ConfigFileParser_createModelFromConfigFileEx (
        "../tests/data/simpleIO_direct_control.cfg");

    IedModel* model2 = ConfigFileParser_createModelFromConfigFileEx (
        "../tests/data/simpleIO_direct_control.cfg");

    IedServer server1 = IedServer_create (model1);

    IedServer server2 = IedServer_create (model2);

    IedServer_start (server1, 10002);
    IedServer_start (server2, 10003);

    iec61850->start ();

    Thread_sleep (1000);

    auto start = std::chrono::high_resolution_clock::now ();
    auto timeout = std::chrono::seconds (10);
    while (!iec61850->m_client->m_active_connection || !iec61850->m_client->m_active_connection->m_connection || IedConnection_getState (
               iec61850->m_client->m_active_connection->m_connection)
           != IED_STATE_CONNECTED)
    {
        auto now = std::chrono::high_resolution_clock::now ();
        if (now - start > timeout)
        {
            IedServer_stop (server1);
            IedServer_destroy (server1);
            IedServer_stop (server2);
            IedServer_destroy (server2);
            IedModel_destroy (model1);
            IedModel_destroy (model2);

            FAIL () << "Connection not established within timeout";
            break;
        }
        Thread_sleep (10);
    }

    ASSERT_EQ (iec61850->m_client->m_active_connection->m_tcpPort, 10002);

    IedServer_stop (server1);

    while (iec61850->m_client->m_active_connection
           && iec61850->m_client->m_active_connection->m_tcpPort == 10002)
    {
        Thread_sleep (10);
    }

    start = std::chrono::high_resolution_clock::now ();
    timeout = std::chrono::seconds (20);
    while (!iec61850->m_client->m_active_connection)
    {
        auto now = std::chrono::high_resolution_clock::now ();
        if (now - start > timeout)
        {
            IedServer_stop (server1);
            IedServer_destroy (server1);
            IedServer_stop (server2);
            IedServer_destroy (server2);
            IedModel_destroy (model1);
            IedModel_destroy (model2);

            FAIL () << "Connection not established within timeout";
            break;
        }
        Thread_sleep (10);
    }

    while (!iec61850->m_client->m_active_connection->m_connection)
    {
        auto now = std::chrono::high_resolution_clock::now ();
        if (now - start > timeout)
        {
            IedServer_stop (server1);
            IedServer_destroy (server1);
            IedServer_stop (server2);
            IedServer_destroy (server2);
            IedModel_destroy (model1);
            IedModel_destroy (model2);

            FAIL () << "Connection not established within timeout";
            break;
        }
        Thread_sleep (10);
    }

    while (!iec61850->m_client->m_active_connection || !iec61850->m_client->m_active_connection->m_connection || IedConnection_getState (
               iec61850->m_client->m_active_connection->m_connection)
           != IED_STATE_CONNECTED)
    {
        auto now = std::chrono::high_resolution_clock::now ();
        if (now - start > timeout)
        {
            IedServer_stop (server1);
            IedServer_destroy (server1);
            IedServer_stop (server2);
            IedServer_destroy (server2);
            IedModel_destroy (model1);
            IedModel_destroy (model2);

            FAIL () << "Connection not established within timeout";
            break;
        }
        Thread_sleep (10);
    }

    Thread_sleep (1000);

    ASSERT_EQ (iec61850->m_client->m_active_connection->m_tcpPort, 10003);

    IedServer_stop (server2);

    IedServer_start (server1, 10002);

    ASSERT_TRUE (IedServer_isRunning (server1));

    Thread_sleep (20000);

    ASSERT_NE (iec61850->m_client->m_active_connection, nullptr);
    ASSERT_NE (iec61850->m_client->m_active_connection->m_connection, nullptr);

    ASSERT_EQ (IedConnection_getState (
                   iec61850->m_client->m_active_connection->m_connection),
               IED_STATE_CONNECTED);

    ASSERT_EQ (iec61850->m_client->m_active_connection->m_tcpPort, 10002);

    IedServer_stop (server1);
    IedServer_destroy (server1);
    IedServer_stop (server2);
    IedServer_destroy (server2);
    IedModel_destroy (model1);
    IedModel_destroy (model2);
}
