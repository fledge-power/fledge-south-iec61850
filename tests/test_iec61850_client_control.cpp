#include <config_category.h>
#include <gtest/gtest.h>
#include <iec61850.hpp>
#include <plugin_api.h>
#include <string.h>
#include "libiec61850/iec61850_server.h"

#include <boost/thread.hpp>
#include <utility>
#include <vector>
#include <libiec61850/hal_thread.h>
#include <optional>

using namespace std;

#define TEST_PORT 2404

// PLUGIN DEFAULT PROTOCOL STACK CONF
static string protocol_config = QUOTE({
    "protocol_stack" : {
        "name" : "iec61850client",
        "version" : "0.0.1",
        "transport_layer" : {
            "ied_name" : "IED1",
            "connections" : [
                {
                    "ip_addr" : "127.0.0.1",
                    "port" : 10002
                }
            ]
        },
        "application_layer" : {
            "polling_interval" : 0
        }
    }
});

// PLUGIN DEFAULT EXCHANGED DATA CONF

static string exchanged_data = QUOTE({
 "exchanged_data": {
  "datapoints": [
   {
    "pivot_id": "TS1",
    "label": "TS1",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "simpleIOGenericIO/GGIO1.SPCSO1",
      "cdc": "SpcTyp"
     }
    ]
   },
   {
    "pivot_id": "TS2",
    "label": "TS2",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "simpleIOGenericIO/GGIO1.SPCSO2",
      "cdc": "SpcTyp"
     }
    ]
   },
   {
    "pivot_id": "TS3",
    "label": "TS3",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "simpleIOGenericIO/GGIO1.SPCSO3",
      "cdc": "SpcTyp"
     }
    ]
   },
   {
    "pivot_id": "TS4",
    "label": "TS4",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "simpleIOGenericIO/GGIO1.SPCSO4",
      "cdc": "SpcTyp"
     }
    ]
   }
  ]
 }
});

static string exchanged_data_2 = QUOTE({
 "exchanged_data": {
  "datapoints": [
   {
    "pivot_id": "TS1",
    "label": "TS1",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "TEMPLATELD1/GGIO1.SPCSO1",
      "cdc": "SpcTyp"
     }
    ]
   },
   {
    "pivot_id": "TS2",
    "label": "TS2",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "TEMPLATELD1/GGIO1.SPCSO2",
      "cdc": "SpcTyp"
     }
    ]
   },
   {
    "pivot_id": "TS3",
    "label": "TS3",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "TEMPLATELD1/GGIO1.SPCSO3",
      "cdc": "SpcTyp"
     }
    ]
   },
   {
    "pivot_id": "TS4",
    "label": "TS4",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "TEMPLATELD1/GGIO1.SPCSO4",
      "cdc": "SpcTyp"
     }
    ]
   },
   {
    "pivot_id": "TM1",
    "label": "TM1",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "TEMPLATELD1/GGIO1.AnIn1",
      "cdc": "MvTyp"
     }
    ]
   },
   {
    "pivot_id": "TM2",
    "label": "TM2",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "TEMPLATELD1/GGIO1.AnIn2",
      "cdc": "MvTyp"
     }
    ]
   },
   {
    "pivot_id": "TM3",
    "label": "TM3",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "TEMPLATELD1/GGIO1.AnIn3",
      "cdc": "MvTyp"
     }
    ]
   },
   {
    "pivot_id": "TM4",
    "label": "TM4",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "TEMPLATELD1/GGIO1.AnIn4",
      "cdc": "MvTyp"
     }
    ]
   },
   {
    "pivot_id": "ST1",
    "label": "ST1",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "TEMPLATELD1/GGIO1.BSCSO1",
      "cdc": "BscTyp"
     }
    ]
   },
   {
    "pivot_id": "ST2",
    "label": "ST2",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "TEMPLATELD1/GGIO1.BSCSO2",
      "cdc": "BscTyp"
     }
    ]
   },
   {
    "pivot_id": "ST3",
    "label": "ST3",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "TEMPLATELD1/GGIO1.BSCSO3",
      "cdc": "BscTyp"
     }
    ]
   },
   {
    "pivot_id": "ST4",
    "label": "ST4",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "TEMPLATELD1/GGIO1.BSCSO4",
      "cdc": "BscTyp"
     }
    ]
   },
   {
    "pivot_id": "IN1",
    "label": "IN1",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "TEMPLATELD1/GGIO1.IntIn1",
      "cdc": "InsTyp"
     }
    ]
   },
   {
    "pivot_id": "IN2",
    "label": "IN2",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "TEMPLATELD1/GGIO1.IntIn2",
      "cdc": "InsTyp"
     }
    ]
   },
   {
    "pivot_id": "IN3",
    "label": "IN3",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "TEMPLATELD1/GGIO1.IntIn3",
      "cdc": "InsTyp"
     }
    ]
   },
   {
    "pivot_id": "IN4",
    "label": "IN4",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "TEMPLATELD1/GGIO1.IntIn4",
      "cdc": "InsTyp"
     }
    ]
   },
   {
    "pivot_id": "AL1",
    "label": "AL1",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "TEMPLATELD1/GGIO1.Alm1",
      "cdc": "SpsTyp"
     }
    ]
   },
   {
    "pivot_id": "AL2",
    "label": "AL2",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "TEMPLATELD1/GGIO1.Alm2",
      "cdc": "SpsTyp"
     }
    ]
   },
   {
    "pivot_id": "AL3",
    "label": "AL3",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "TEMPLATELD1/GGIO1.Alm3",
      "cdc": "SpsTyp"
     }
    ]
   },
   {
    "pivot_id": "AL4",
    "label": "AL4",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "TEMPLATELD1/GGIO1.Alm4",
      "cdc": "SpsTyp"
     }
    ]
   },
   {
    "pivot_id": "AN1",
    "label": "AN1",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "TEMPLATELD1/GGIO1.AnOut1",
      "cdc": "ApcTyp"
     }
    ]
   },
   {
    "pivot_id": "AN2",
    "label": "AN2",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "TEMPLATELD1/GGIO1.AnOut2",
      "cdc": "ApcTyp"
     }
    ]
   },
   {
    "pivot_id": "AN3",
    "label": "AN3",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "TEMPLATELD1/GGIO1.AnOut3",
      "cdc": "ApcTyp"
     }
    ]
   },
   {
    "pivot_id": "AN4",
    "label": "AN4",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "TEMPLATELD1/GGIO1.AnOut4",
      "cdc": "ApcTyp"
     }
    ]
   },
   {
    "pivot_id": "DP1",
    "label": "DP1",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "TEMPLATELD1/GGIO1.DPCSO1",
      "cdc": "DpcTyp"
     }
    ]
   },
   {
    "pivot_id": "DP2",
    "label": "DP2",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "TEMPLATELD1/GGIO1.DPCSO2",
      "cdc": "DpcTyp"
     }
    ]
   },
   {
    "pivot_id": "DP3",
    "label": "DP3",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "TEMPLATELD1/GGIO1.DPCSO3",
      "cdc": "DpcTyp"
     }
    ]
   },
   {
    "pivot_id": "DP4",
    "label": "DP4",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "TEMPLATELD1/GGIO1.DPCSO4",
      "cdc": "DpcTyp"
     }
    ]
   }
  ]
 }
});

static string exchanged_data_3 = QUOTE({
 "exchanged_data": {
  "datapoints": [
   {
    "pivot_id": "SG1",
    "label": "SG1",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "DER_Scheduler_Control/ActPow_FSCH01.SchdReuse",
      "cdc": "SpgTyp"
     }
    ]
   },
   {
    "pivot_id": "SG2",
    "label": "SG2",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "DER_Scheduler_Control/ActPow_FSCH01.ValASG001",
      "cdc": "AsgTyp"
     }
    ]
   },
    {
    "pivot_id": "SG3",
    "label": "SG3",
    "protocols": [
     {
      "name": "iec61850",
      "objref": "DER_Scheduler_Control/ActPow_FSCH01.SchdPrio",
      "cdc": "IngTyp"
     }
    ]
   }

  ]
 }
});


// PLUGIN DEFAULT TLS CONF
static string tls_config = QUOTE({
    "tls_conf" : {
        "private_key" : "server-key.pem",
        "own_cert" : "server.cer",
        "ca_certs" : [
            {
                "cert_file" : "root.cer"
            }
        ]
    }
});

static string tls_config_2 = QUOTE({
    "tls_conf" : {
        "private_key" : "iec104_client.key",
        "own_cert" : "iec104_client.cer",
        "ca_certs" : [
            {
                "cert_file" : "iec104_ca.cer"
            }
        ],
        "remote_certs" : [
            {
                "cert_file" : "iec104_server.cer"
            }
        ]
    }
});

class ControlTest : public testing::Test
{
protected:
    IEC61850 *iec61850 = nullptr;
    int ingestCallbackCalled = 0;
    Reading *storedReading = nullptr;
    int clockSyncHandlerCalled = 0;
    std::vector<Reading *> storedReadings;

    int asduHandlerCalled = 0;
    IedConnection lastConnection = nullptr;
    int lastOA = 0;
    int openConnections = 0;
    int activations = 0;
    int deactivations = 0;
    int maxConnections = 0;

    void SetUp() override
    {
        iec61850 = new IEC61850();

        iec61850->registerIngest(this, ingestCallback);
    }

    void TearDown() override
    {
        iec61850->stop();
        delete iec61850;

        for (auto reading : storedReadings)
        {
            delete reading;
        }
    }

    static bool hasChild(Datapoint &dp, std::string childLabel)
    {
        DatapointValue &dpv = dp.getData();

        auto dps = dpv.getDpVec();

        for (auto sdp : *dps)
        {
            if (sdp->getName() == childLabel)
            {
                return true;
            }
        }

        return false;
    }

    static Datapoint *getChild(Datapoint &dp, std::string childLabel)
    {
        DatapointValue &dpv = dp.getData();

        auto dps = dpv.getDpVec();

        for (Datapoint *childDp : *dps)
        {
            if (childDp->getName() == childLabel)
            {
                return childDp;
            }
        }

        return nullptr;
    }

    static int64_t getIntValue(Datapoint *dp)
    {
        DatapointValue dpValue = dp->getData();
        return dpValue.toInt();
    }

    static std::string getStrValue(Datapoint *dp)
    {
        return dp->getData().toStringValue();
    }

    static bool hasObject(Reading &reading, std::string label)
    {
        std::vector<Datapoint *> dataPoints = reading.getReadingData();

        for (Datapoint *dp : dataPoints)
        {
            if (dp->getName() == label)
            {
                return true;
            }
        }

        return false;
    }

    static Datapoint *getObject(Reading &reading, std::string label)
    {
        std::vector<Datapoint *> dataPoints = reading.getReadingData();

        for (Datapoint *dp : dataPoints)
        {
            if (dp->getName() == label)
            {
                return dp;
            }
        }

        return nullptr;
    }

    static void ingestCallback(void *parameter, Reading reading)
    {
        auto self = (ControlTest *)parameter;

        printf("ingestCallback called -> asset: (%s)\n", reading.getAssetName().c_str());

        std::vector<Datapoint *> dataPoints = reading.getReadingData();

        for (Datapoint *sdp : dataPoints)
        {
            printf("name: %s value: %s\n", sdp->getName().c_str(), sdp->getData().toString().c_str());
        }
        self->storedReading = new Reading(reading);

        self->storedReadings.push_back(self->storedReading);

        self->ingestCallbackCalled++;
    }

    void verifyDatapoint(Datapoint* parent, const std::string& childName, const int* expectedValue) {
        ASSERT_NE(parent, nullptr) << "Parent Datapoint is nullptr";

        Datapoint* child = getChild(*parent, childName);
        ASSERT_NE(child, nullptr) << "Child Datapoint '" << childName << "' is nullptr";

        if (expectedValue != nullptr) {
            ASSERT_EQ(child->getData().toInt(), *expectedValue) << "Int value of Datapoint '" << childName << "' does not match expected value";
        }
    }

    void verifyDatapoint(Datapoint* parent, const std::string& childName, const double* expectedValue) {
        ASSERT_NE(parent, nullptr) << "Parent Datapoint is nullptr";

        Datapoint* child = getChild(*parent, childName);
        ASSERT_NE(child, nullptr) << "Child Datapoint '" << childName << "' is nullptr";

        if (expectedValue != nullptr) {
            ASSERT_EQ(child->getData().toDouble(), *expectedValue) << "Double value of Datapoint '" << childName << "' does not match expected value";
        }
    }

    void verifyDatapoint(Datapoint* parent, const std::string& childName, const std::string* expectedValue) {
        ASSERT_NE(parent, nullptr) << "Parent Datapoint is nullptr";

        Datapoint* child = getChild(*parent, childName);
        ASSERT_NE(child, nullptr) << "Child Datapoint '" << childName << "' is nullptr";

        if (expectedValue != nullptr) {
            ASSERT_EQ(child->getData().toString(), *expectedValue) << "String value of Datapoint '" << childName << "' does not match expected value";
        }
    }

    

    void verifyDatapoint(Datapoint* parent, const std::string& childName) {
        ASSERT_NE(parent, nullptr) << "Parent Datapoint is nullptr";

        Datapoint* child = getChild(*parent, childName);
        ASSERT_NE(child, nullptr) << "Child Datapoint '" << childName << "' is nullptr";
    }
};


static ControlHandlerResult
controlHandlerForBinaryOutput(ControlAction action, void* parameter, MmsValue* value, bool test)
{
    uint64_t timestamp = Hal_getTimeInMs();

    printf("control handler called\n");
    printf("  ctlNum: %i\n", ControlAction_getCtlNum(action));

    auto pair = (std::pair<IedServer, DataAttribute*>*) parameter;

    ClientConnection clientCon = ControlAction_getClientConnection(action);

    if (clientCon) {
        printf("Control from client %s\n", ClientConnection_getPeerAddress(clientCon));
        IedServer_updateAttributeValue(pair->first, pair->second, value);
        return CONTROL_RESULT_OK;
    }

    return CONTROL_RESULT_FAILED;
}

TEST_F(ControlTest, SingleCommandDirectNormal) {
    iec61850->setJsonConfig(protocol_config, exchanged_data, tls_config);

    IedModel* model = ConfigFileParser_createModelFromConfigFileEx("../tests/data/simpleIO_control_tests.cfg");
    IedServer server = IedServer_create(model);
    IedServer_start(server,10002);

    iec61850->start();
    Thread_sleep(1000); 

    auto start = std::chrono::high_resolution_clock::now();
    auto timeout = std::chrono::seconds(10);  
    while (IedConnection_getState(iec61850->m_client->m_active_connection->m_connection) != IED_STATE_CONNECTED) {
        auto now = std::chrono::high_resolution_clock::now();
        if (now - start > timeout) {
            IedServer_stop(server);
            IedServer_destroy(server);
            IedModel_destroy(model);
            FAIL() << "Connection not established within timeout";
            break;
        }
        Thread_sleep(10); 
    }

    auto params = new PLUGIN_PARAMETER*[1];
    params[0] = new PLUGIN_PARAMETER;
    params[0]->name = std::string("Pivot");
    params[0]->value = std::string(R"({"GTIC":{"ComingFrom":"iec61850", "SpcTyp":{"q":{"test":0}, "t":{"SecondSinceEpoch":1700566837, "FractionOfSecond":15921577}, "ctlVal":0}, "Identifier":"TS1", "Select":{"stVal":0}}})");
    iec61850->operation("PivotCommand", 1, params);

    delete params[0];
    delete[] params;

    timeout = std::chrono::seconds(3);  
    start = std::chrono::high_resolution_clock::now();
    while (ingestCallbackCalled != 1) {
        auto now = std::chrono::high_resolution_clock::now();
        if (now - start > timeout) {
            IedServer_stop(server);
            IedServer_destroy(server);
            IedModel_destroy(model);
            FAIL() << "Callback not called within timeout";
            break;
        }
        Thread_sleep(10); 
    }

    ASSERT_NE(storedReading, nullptr);
    ASSERT_EQ(storedReading->getDatapointCount(), 1);
    Datapoint* commandResponse = storedReading->getReadingData()[0];
    verifyDatapoint(commandResponse, "GTIC");
    Datapoint* gtic = getChild(*commandResponse,"GTIC");

    verifyDatapoint(gtic, "Cause");
    Datapoint* cause = getChild(*gtic,"Cause");

    int expectedStVal = 7;
    verifyDatapoint(cause, "stVal", &expectedStVal);

    IedServer_stop(server);
    IedServer_destroy(server);
    IedModel_destroy(model);
}

TEST_F(ControlTest, SingleCommandDirectEnhanced) {
    iec61850->setJsonConfig(protocol_config, exchanged_data, tls_config);

    IedModel* model = ConfigFileParser_createModelFromConfigFileEx("../tests/data/simpleIO_control_tests.cfg");
    IedServer server = IedServer_create(model);
    IedServer_start(server,10002);
    iec61850->start();
    Thread_sleep(1000); 

    auto start = std::chrono::high_resolution_clock::now();
    auto timeout = std::chrono::seconds(10);  
    while (IedConnection_getState(iec61850->m_client->m_active_connection->m_connection) != IED_STATE_CONNECTED) {
        auto now = std::chrono::high_resolution_clock::now();
        if (now - start > timeout) {
            IedServer_stop(server);
            IedServer_destroy(server);
            IedModel_destroy(model);
            FAIL() << "Connection not established within timeout";
            break;
        }
        Thread_sleep(10); 
    }

    auto params = new PLUGIN_PARAMETER*[1];
    params[0] = new PLUGIN_PARAMETER;
    params[0]->name = std::string("Pivot");
    params[0]->value = std::string(R"({"GTIC":{"ComingFrom":"iec61850", "SpcTyp":{"q":{"test":0}, "t":{"SecondSinceEpoch":1700566837, "FractionOfSecond":15921577}, "ctlVal":1}, "Identifier":"TS3", "Select":{"stVal":0}}})");
    iec61850->operation("PivotCommand", 1, params);

    delete params[0];
    delete[] params;

    timeout = std::chrono::seconds(3);  
    start = std::chrono::high_resolution_clock::now();
    while (ingestCallbackCalled != 2) {
        auto now = std::chrono::high_resolution_clock::now();
        if (now - start > timeout) {
            IedServer_stop(server);
            IedServer_destroy(server);
            IedModel_destroy(model);
            FAIL() << "Callback not called within timeout";
        }
        Thread_sleep(10); 
    }

    ASSERT_FALSE(storedReadings.empty());
    ASSERT_EQ(storedReadings.size(), 2);
    Datapoint* commandResponse = storedReadings[0]->getReadingData()[0];
    verifyDatapoint(commandResponse, "GTIC");
    Datapoint* gtic = getChild(*commandResponse,"GTIC");

    verifyDatapoint(gtic, "Cause");
    Datapoint* cause = getChild(*gtic,"Cause");

    int expectedStVal = 7;
    verifyDatapoint(cause, "stVal", &expectedStVal);

    Datapoint* commandResponse2 = storedReadings[1]->getReadingData()[0];
    verifyDatapoint(commandResponse2, "GTIC");
    Datapoint* gtic2 = getChild(*commandResponse2,"GTIC");

    verifyDatapoint(gtic2, "Cause");
    Datapoint* cause2 = getChild(*gtic2,"Cause");

    expectedStVal = 10;
    verifyDatapoint(cause2, "stVal", &expectedStVal);

    IedServer_stop(server);
    IedServer_destroy(server);
    IedModel_destroy(model);
}

TEST_F(ControlTest, SingleCommandSetValue) {
    iec61850->setJsonConfig(protocol_config, exchanged_data, tls_config);

    IedModel* model = ConfigFileParser_createModelFromConfigFileEx("../tests/data/simpleIO_control_tests.cfg");
    IedServer server = IedServer_create(model);
    IedServer_start(server,10002);

    iec61850->start();
    Thread_sleep(1000); 

    auto start = std::chrono::high_resolution_clock::now();
    auto timeout = std::chrono::seconds(10);  
    while (IedConnection_getState(iec61850->m_client->m_active_connection->m_connection) != IED_STATE_CONNECTED) {
        auto now = std::chrono::high_resolution_clock::now();
        if (now - start > timeout) {
            IedServer_stop(server);
            IedServer_destroy(server);
            IedModel_destroy(model);
            FAIL() << "Connection not established within timeout";
            break;
        }
        Thread_sleep(10); 
    }

    auto IEDMODEL_GenericIO_GGIO1_SPCSO1 = (DataObject*) IedModel_getModelNodeByObjectReference(model, "simpleIOGenericIO/GGIO1.SPCSO1");

    auto pair = new std::pair<IedServer, DataAttribute*>(server, (DataAttribute*) IedModel_getModelNodeByObjectReference(model, "simpleIOGenericIO/GGIO1.SPCSO1.stVal"));
    IedServer_setControlHandler(server, IEDMODEL_GenericIO_GGIO1_SPCSO1 , (ControlHandler) controlHandlerForBinaryOutput, pair);
    
    IedClientError err;
    
    MmsValue* mmsValue = iec61850->m_client->m_active_connection->readValue(&err, "simpleIOGenericIO/GGIO1.SPCSO1.stVal", IEC61850_FC_ST);
    ASSERT_FALSE(MmsValue_getBoolean(mmsValue));
    MmsValue_delete(mmsValue);

    auto params = new PLUGIN_PARAMETER*[1];
    params[0] = new PLUGIN_PARAMETER;
    params[0]->name = std::string("Pivot");
    params[0]->value = std::string(R"({"GTIC":{"ComingFrom":"iec61850", "SpcTyp":{"q":{"test":0}, "t":{"SecondSinceEpoch":1700566837, "FractionOfSecond":15921577}, "ctlVal":1}, "Identifier":"TS1", "Select":{"stVal":0}}})");
    bool res = iec61850->operation("PivotCommand", 1, params);

    delete params[0];
    delete[] params;

    timeout = std::chrono::seconds(3);  
    start = std::chrono::high_resolution_clock::now();
    while (ingestCallbackCalled != 1) {
        auto now = std::chrono::high_resolution_clock::now();
        if (now - start > timeout) {
            IedServer_stop(server);
            IedServer_destroy(server);
            IedModel_destroy(model);
            FAIL() << "Callback not called within timeout";
        }
        Thread_sleep(10); 
    }

    mmsValue = iec61850->m_client->m_active_connection->readValue(&err, "simpleIOGenericIO/GGIO1.SPCSO1.stVal", IEC61850_FC_ST);

    ASSERT_TRUE(mmsValue && MmsValue_getBoolean(mmsValue)); 

    MmsValue_delete(mmsValue);
   
    IedServer_stop(server);
    IedServer_destroy(server);
    IedModel_destroy(model);
    delete pair;
}

TEST_F(ControlTest, DoubleCommandDirectNormal) {
    iec61850->setJsonConfig(protocol_config, exchanged_data_2 , tls_config);

    IedModel* model = ConfigFileParser_createModelFromConfigFileEx("../tests/data/iec61850fledgetest.cfg");
    IedServer server = IedServer_create(model);
    IedServer_start(server,10002);

    iec61850->start();
    Thread_sleep(1000); 

    auto start = std::chrono::high_resolution_clock::now();
    auto timeout = std::chrono::seconds(10);  
    while (IedConnection_getState(iec61850->m_client->m_active_connection->m_connection) != IED_STATE_CONNECTED) {
        auto now = std::chrono::high_resolution_clock::now();
        if (now - start > timeout) {
            IedServer_stop(server);
            IedServer_destroy(server);
            IedModel_destroy(model);
            FAIL() << "Connection not established within timeout";
            break;
        }
        Thread_sleep(10); 
    }

    auto params = new PLUGIN_PARAMETER*[1];
    params[0] = new PLUGIN_PARAMETER;
    params[0]->name = std::string("Pivot");
    params[0]->value = std::string(R"({"GTIC":{"ComingFrom":"iec61850", "DpcTyp":{"q":{"test":0}, "t":{"SecondSinceEpoch":1700566837, "FractionOfSecond":15921577}, "ctlVal":0}, "Identifier":"TS1", "Select":{"stVal":0}}})");
    iec61850->operation("PivotCommand", 1, params);

    delete params[0];
    delete[] params;

    timeout = std::chrono::seconds(3);  
    start = std::chrono::high_resolution_clock::now();
    while (ingestCallbackCalled != 1) {
        auto now = std::chrono::high_resolution_clock::now();
        if (now - start > timeout) {
            IedServer_stop(server);
            IedServer_destroy(server);
            IedModel_destroy(model);
            FAIL() << "Callback not called within timeout";
            break;
        }
        Thread_sleep(10); 
    }

    ASSERT_NE(storedReading, nullptr);
    ASSERT_EQ(storedReading->getDatapointCount(), 1);
    Datapoint* commandResponse = storedReading->getReadingData()[0];
    verifyDatapoint(commandResponse, "GTIC");
    Datapoint* gtic = getChild(*commandResponse,"GTIC");

    verifyDatapoint(gtic, "Cause");
    Datapoint* cause = getChild(*gtic,"Cause");

    int expectedStVal = 7;
    verifyDatapoint(cause, "stVal", &expectedStVal);

    IedServer_stop(server);
    IedServer_destroy(server);
    IedModel_destroy(model);
}

TEST_F(ControlTest, AnalogueCommandDirectNormal) {
    iec61850->setJsonConfig(protocol_config, exchanged_data_2 , tls_config);

    IedModel* model = ConfigFileParser_createModelFromConfigFileEx("../tests/data/iec61850fledgetest.cfg");
    IedServer server = IedServer_create(model);
    IedServer_start(server,10002);

    iec61850->start();
    Thread_sleep(1000); 

    auto start = std::chrono::high_resolution_clock::now();
    auto timeout = std::chrono::seconds(10);  
    while (IedConnection_getState(iec61850->m_client->m_active_connection->m_connection) != IED_STATE_CONNECTED) {
        auto now = std::chrono::high_resolution_clock::now();
        if (now - start > timeout) {
            IedServer_stop(server);
            IedServer_destroy(server);
            IedModel_destroy(model);
            FAIL() << "Connection not established within timeout";
            break;
        }
        Thread_sleep(10); 
    }

    auto params = new PLUGIN_PARAMETER*[1];
    params[0] = new PLUGIN_PARAMETER;
    params[0]->name = std::string("Pivot");
    params[0]->value = std::string(R"({"GTIC":{"ComingFrom":"iec61850", "ApcTyp":{"q":{"test":0}, "t":{"SecondSinceEpoch":1700566837, "FractionOfSecond":15921577}, "ctlVal":0.2}, "Identifier":"TS1", "Select":{"stVal":0}}})");
    iec61850->operation("PivotCommand", 1, params);

    delete params[0];
    delete[] params;

    timeout = std::chrono::seconds(3);  
    start = std::chrono::high_resolution_clock::now();
    while (ingestCallbackCalled != 1) {
        auto now = std::chrono::high_resolution_clock::now();
        if (now - start > timeout) {
            IedServer_stop(server);
            IedServer_destroy(server);
            IedModel_destroy(model);
            FAIL() << "Callback not called within timeout";
            break;
        }
        Thread_sleep(10); 
    }

    ASSERT_NE(storedReading, nullptr);
    ASSERT_EQ(storedReading->getDatapointCount(), 1);
    Datapoint* commandResponse = storedReading->getReadingData()[0];
    verifyDatapoint(commandResponse, "GTIC");
    Datapoint* gtic = getChild(*commandResponse,"GTIC");

    verifyDatapoint(gtic, "Cause");
    Datapoint* cause = getChild(*gtic,"Cause");

    int expectedStVal = 7;
    verifyDatapoint(cause, "stVal", &expectedStVal);

    IedServer_stop(server);
    IedServer_destroy(server);
    IedModel_destroy(model);
}

TEST_F(ControlTest, StepCommandDirectNormal) {
    iec61850->setJsonConfig(protocol_config, exchanged_data_2 , tls_config);

    IedModel* model = ConfigFileParser_createModelFromConfigFileEx("../tests/data/iec61850fledgetest.cfg");
    IedServer server = IedServer_create(model);
    IedServer_start(server,10002);

    iec61850->start();
    Thread_sleep(1000); 

    auto start = std::chrono::high_resolution_clock::now();
    auto timeout = std::chrono::seconds(10);  
    while (IedConnection_getState(iec61850->m_client->m_active_connection->m_connection) != IED_STATE_CONNECTED) {
        auto now = std::chrono::high_resolution_clock::now();
        if (now - start > timeout) {
            IedServer_stop(server);
            IedServer_destroy(server);
            IedModel_destroy(model);
            FAIL() << "Connection not established within timeout";
            break;
        }
        Thread_sleep(10); 
    }

    auto params = new PLUGIN_PARAMETER*[1];
    params[0] = new PLUGIN_PARAMETER;
    params[0]->name = std::string("Pivot");
    params[0]->value = std::string(R"({"GTIC":{"ComingFrom":"iec61850", "BscTyp":{"q":{"test":0}, "t":{"SecondSinceEpoch":1700566837, "FractionOfSecond":15921577}, "ctlVal":"lower"}, "Identifier":"TS1", "Select":{"stVal":0}}})");
    iec61850->operation("PivotCommand", 1, params);

    delete params[0];
    delete[] params;

    timeout = std::chrono::seconds(3);  
    start = std::chrono::high_resolution_clock::now();
    while (ingestCallbackCalled != 1) {
        auto now = std::chrono::high_resolution_clock::now();
        if (now - start > timeout) {
            IedServer_stop(server);
            IedServer_destroy(server);
            IedModel_destroy(model);
            FAIL() << "Callback not called within timeout";
            break;
        }
        Thread_sleep(10); 
    }

    ASSERT_NE(storedReading, nullptr);
    ASSERT_EQ(storedReading->getDatapointCount(), 1);
    Datapoint* commandResponse = storedReading->getReadingData()[0];
    verifyDatapoint(commandResponse, "GTIC");
    Datapoint* gtic = getChild(*commandResponse,"GTIC");

    verifyDatapoint(gtic, "Cause");
    Datapoint* cause = getChild(*gtic,"Cause");

    int expectedStVal = 7;
    verifyDatapoint(cause, "stVal", &expectedStVal);

    IedServer_stop(server);
    IedServer_destroy(server);
    IedModel_destroy(model);
}

TEST_F(ControlTest, WriteOperations) {
    iec61850->setJsonConfig(protocol_config, exchanged_data_3 , tls_config);

    IedModel* model = ConfigFileParser_createModelFromConfigFileEx("../tests/data/schedulermodel.cfg");
    IedServer server = IedServer_create(model);
    IedServer_start(server,10002);

    iec61850->start();
    Thread_sleep(1000); 

    auto start = std::chrono::high_resolution_clock::now();
    auto timeout = std::chrono::seconds(10);  
    while (IedConnection_getState(iec61850->m_client->m_active_connection->m_connection) != IED_STATE_CONNECTED) {
        auto now = std::chrono::high_resolution_clock::now();
        if (now - start > timeout) {
            IedServer_stop(server);
            IedServer_destroy(server);
            IedModel_destroy(model);
            FAIL() << "Connection not established within timeout";
            break;
        }
        Thread_sleep(10); 
    }

    auto params = new PLUGIN_PARAMETER*[1];
    params[0] = new PLUGIN_PARAMETER;
    params[0]->name = std::string("Pivot");
    params[0]->value = std::string(R"({"GTIC":{"ComingFrom":"iec61850", "IngTyp":{"setVal":1}, "Identifier":"SG3"}})");
    iec61850->operation("PivotCommand", 1, params);

    delete params[0];
    delete[] params;

    IedServer_stop(server);
    IedServer_destroy(server);
    IedModel_destroy(model);
}