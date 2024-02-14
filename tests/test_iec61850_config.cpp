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

static string wrong_protocol_config_1 = QUOTE({
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
        }
        "application_layer" : {
            "polling_interval" : 0
        }
    }
});

static string wrong_protocol_config_2 = QUOTE({});

static string wrong_protocol_config_3 = QUOTE({
    "protocol_stack" : {
        "name" : "iec61850client",
        "version" : "0.0.1",
        "application_layer" : {
            "polling_interval" : 0
        }
    }
});

static string wrong_protocol_config_4 = QUOTE({
    "protocol_stack" : {
        "name" : "iec61850client",
        "version" : "0.0.1",
        "transport_layer" : {
            "ied_name" : "IED1"
        },
        "application_layer" : {
            "polling_interval" : 0
        }
    }
});

static string wrong_protocol_config_5 = QUOTE({
    "protocol_stack" : {
        "name" : "iec61850client",
        "version" : "0.0.1",
        "transport_layer" : {
            "ied_name" : "IED1",
            "connections" : [
                {
                    "ip_addr" : "",
                    "port" : 10002
                }
            ]
        },
        "application_layer" : {
            "polling_interval" : 0
        }
    }
});

static string wrong_protocol_config_6 = QUOTE({
    "protocol_stack" : {
        "name" : "iec61850client",
        "version" : "0.0.1",
        "transport_layer" : {
            "ied_name" : "IED1",
            "connections" : [
                {
                    "ip_addr" : "127.0.0.1",
                    "port" : -1
                }
            ]
        },
        "application_layer" : {
            "polling_interval" : 0
        }
    }
});

static string osi_protocol_config = QUOTE({
    "protocol_stack" : {
        "name" : "iec61850client",
        "version" : "0.0.1",
        "transport_layer" : {
            "ied_name" : "IED1",
            "connections" : [
                {
                    "ip_addr" : "127.0.0.1",
                    "port" : 10002,
                    "osi": {
                        "local_ap_title":"1,3,9999.13",
                        "local_ae_qualifier":12,
                        "remote_ap_title":"1,2,1200,15,3",
                        "remote_ae_qualifier":1,
                        "local_psel":"0x12,0x34,0x56,0x78",
                        "local_ssel":"0x04,0x01,0x02,0x03,0x04",
                        "local_tsel":"0x00,0x01,0x02",
                        "remote_psel":"0x87,0x65,0x43,0x21",
                        "remote_ssel":"0x00,0x01",
                        "remote_tsel":"0x00,0x01"
                    }
                }
            ]
        },
        "application_layer" : {
            "polling_interval" : 0
        }
    }
});

static string wrong_protocol_config_7 = QUOTE({
    "protocol_stack" : {
        "name" : "iec61850client",
        "version" : "0.0.1",
        "transport_layer" : {
            "ied_name" : "IED1",
            "connections" : [
                {
                    "ip_addr" : "127.0.0.1",
                    "port" : 10002,
                    "tls" : "false"
                }
            ]
        },
        "application_layer" : {
            "polling_interval" : 0
        }
    }
});

static string wrong_protocol_config_8 = QUOTE({
    "protocol_stack" : {
        "name" : "iec61850client",
        "version" : "0.0.1",
        "transport_layer" : {
            "ied_name" : "IED1",
            "connections" : [
                {
                    "ip_addr" : "127.0.0.1",
                    "port" : 10002,
                    "tls" : false
                }
            ]
        }
    }
});

static string wrong_protocol_config_9 = QUOTE({
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
            "polling_interval" : "0"
        }
    }
});

static string wrong_protocol_config_10 = QUOTE({
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
            "polling_interval" : -1
        }
    }
});

static string wrong_protocol_config_11 = QUOTE({
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
            "polling_interval" : 0,
            "datasets" : [{}]
        }
    }
});

static string wrong_protocol_config_12 = QUOTE({
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
            "polling_interval" : 0,
            "datasets" : [
                {
                    "dataset_ref" : "simpleIOGenericIO/LLN0.Mags",
                    "entries" : [
                        "simpleIOGenericIO/GGIO1.AnIn1[MX]",
                        "simpleIOGenericIO/GGIO1.AnIn2[MX]",
                        "simpleIOGenericIO/GGIO1.AnIn3[MX]",
                        "simpleIOGenericIO/GGIO1.AnIn4[MX]"
                    ]
                }
            ]
        }
    }
});

static string wrong_protocol_config_13 = QUOTE({
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
            "polling_interval" : 0,
            "report_subscriptions" : [
                {},
                {
                    "rcb_ref" : "simpleIOGenericIO/LLN0.RP.EventsIndexed",
                    "dataset_ref" : "simpleIOGenericIO/LLN0.Events2",
                    "trgops" : [
                        "dchg",
                        "qchg",
                        "gi"
                    ],
                    "gi" : false
                }
            ]
        }
    }
});

static string wrong_protocol_config_14 = QUOTE({
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
            "polling_interval" : 0,
            "report_subscriptions" : [
                "data",
                {
                    "rcb_ref" : "simpleIOGenericIO/LLN0.RP.EventsIndexed",
                    "dataset_ref" : "simpleIOGenericIO/LLN0.Events2",
                    "trgops" : [
                        "dchg",
                        "qchg",
                        "gi"
                    ],
                    "gi" : false
                }
            ]
        }
    }
});

static string wrong_protocol_config_15 = QUOTE({
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
            "polling_interval" : 0,
            "report_subscriptions" : [
                "data",
                {
                    "rcb_ref" : "simpleIOGenericIO/LLN0.RP.EventsIndexed",
                    "trgops" : [
                        "dchg",
                        "qchg",
                        "gi"
                    ],
                    "gi" : false
                }
            ]
        }
    }
});

static string wrong_protocol_config_16 = QUOTE({
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
            "polling_interval" : 0,
            "report_subscriptions" : [
                {
                    "rcb_ref" : "simpleIOGenericIO/LLN0.RP.EventsRCB",
                    "dataset_ref" : "simpleIOGenericIO/LLN0.Mags",
                    "trgops" : [
                        "gi"
                    ],
                    "gi" : true
                },
                {
                    "rcb_ref" : "simpleIOGenericIO/LLN0.RP.EventsIndexed",
                    "dataset_ref" : "simpleIOGenericIO/LLN0.Events2",
                    "gi" : true
                }
            ]
        }
    }
});

static string wrong_protocol_config_17 = QUOTE({
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
            ],
            "tls" : false
        },
        "application_layer" : {
            "polling_interval" : 10,
            "datasets" : [
                {
                    "dataset_ref" : "simpleIOGenericIO/LLN0.Mags",
                    "entries" : [
                        "simpleIOGenericIO/GGIO1.AnIn1[MX]",
                        "simpleIOGenericIO/GGIO1.AnIn2[MX]",
                        "simpleIOGenericIO/GGIO1.AnIn3[MX]",
                        "simpleIOGenericIO/GGIO1.AnIn4[MX]",
                        "simpleIOGenericIO/GGIO1.SPCSO1[ST]",
                        "simpleIOGenericIO/GGIO1.SPCSO2[ST]",
                        "simpleIOGenericIO/GGIO1.SPCSO3[ST]",
                        "simpleIOGenericIO/GGIO1.SPCSO4[ST]"
                    ],
                    "dynamic" : true
                },
                {
                    "dataset_ref" : "simpleIOGenericIO/LLN0.Events2",
                    "entries" : [
                        "simpleIOGenericIO/GGIO1.AnIn1.mag.f[MX]",
                        "simpleIOGenericIO/GGIO1.AnIn2.mag.f[MX]",
                        "simpleIOGenericIO/GGIO1.AnIn3.mag.f[MX]",
                        "simpleIOGenericIO/GGIO1.AnIn4.mag.f[MX]"
                    ],
                    "dynamic" : false
                }
            ],
            "report_subscriptions" : [
                {
                    "rcb_ref" : "simpleIOGenericIO/LLN0.RP.EventsRCB",
                    "dataset_ref" : "simpleIOGenericIO/LLN0.Mags",
                    "trgops" : [
                        "dchg",
                        "qchg"
                    ],
                    "buftm": 1,
                    "intgpd": 2
                },
                {
                    "rcb_ref" : "simpleIOGenericIO/LLN0.RP.EventsIndexed",
                    "dataset_ref" : "simpleIOGenericIO/LLN0.Events2",
                    "trgops" : [
                        "dchg",
                        "qchg"
                    ],
                    "buftm": 1,
                    "intgpd": 2
                }
            ]
        }
    }
});

static string exchanged_data = QUOTE({
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

class ConfigTest : public testing::Test
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
        auto self = (ConfigTest *)parameter;

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
};

TEST_F(ConfigTest, getWrongExchangeDefinitionByLabel) {

    IEC61850ClientConfig* config = new IEC61850ClientConfig();

    auto exDefinition = config->getExchangeDefinitionByLabel("wrong");

    ASSERT_EQ(exDefinition,nullptr);
}

TEST_F(ConfigTest, getWrongExchangeDefinitionByPivotID) {

    IEC61850ClientConfig* config = new IEC61850ClientConfig();

    auto exDefinition = config->getExchangeDefinitionByPivotId("wrong");

    ASSERT_EQ(exDefinition,nullptr);
}

TEST_F(ConfigTest, ProtocolConfigParseError) {
    
    IEC61850ClientConfig* config = new IEC61850ClientConfig();

    config->importProtocolConfig(wrong_protocol_config_1);

    ASSERT_FALSE(config->m_protocolConfigComplete);
}

 TEST_F(ConfigTest, ProtocolConfigNoJsonProtocolStack) {
    
     IEC61850ClientConfig* config = new IEC61850ClientConfig();

     config->importProtocolConfig(wrong_protocol_config_2);

     ASSERT_FALSE(config->m_protocolConfigComplete);
 }

TEST_F(ConfigTest, ProtocolConfigNoTransportLayer) {
    
    IEC61850ClientConfig* config = new IEC61850ClientConfig();

    config->importProtocolConfig(wrong_protocol_config_3);

    ASSERT_FALSE(config->m_protocolConfigComplete);
}

TEST_F(ConfigTest, ProtocolConfigNoConnections) {

    IEC61850ClientConfig* config = new IEC61850ClientConfig();

    config->importProtocolConfig(wrong_protocol_config_4);

    ASSERT_FALSE(config->m_protocolConfigComplete);
}

TEST_F(ConfigTest, ProtocolConfigNoConnectionIP) {

    IEC61850ClientConfig* config = new IEC61850ClientConfig();

    config->importProtocolConfig(wrong_protocol_config_5);

    ASSERT_TRUE(config->m_protocolConfigComplete);
}

TEST_F(ConfigTest, ProtocolConfigInvalidConnectionPort) {

    IEC61850ClientConfig* config = new IEC61850ClientConfig();

    config->importProtocolConfig(wrong_protocol_config_6);

    ASSERT_TRUE(config->m_protocolConfigComplete);
}

TEST_F(ConfigTest, ProtocolConfigWithOsi) {

    IEC61850ClientConfig* config = new IEC61850ClientConfig();

    config->importProtocolConfig(osi_protocol_config);

    ASSERT_TRUE(config->m_protocolConfigComplete);
}

TEST_F(ConfigTest, ProtocolConfigTlsNotBoolean) {

    IEC61850ClientConfig* config = new IEC61850ClientConfig();

    config->importProtocolConfig(wrong_protocol_config_7);

    ASSERT_TRUE(config->m_protocolConfigComplete);
}

TEST_F(ConfigTest, ProtocolConfigNoAppLayer) {

    IEC61850ClientConfig* config = new IEC61850ClientConfig();

    config->importProtocolConfig(wrong_protocol_config_8);

    ASSERT_FALSE(config->m_protocolConfigComplete);
}

TEST_F(ConfigTest, ProtocolConfigPollingIntervalNotInt) {

    IEC61850ClientConfig* config = new IEC61850ClientConfig();

    config->importProtocolConfig(wrong_protocol_config_9);

    ASSERT_FALSE(config->m_protocolConfigComplete);
}

TEST_F(ConfigTest, ProtocolConfigWrongPollingInterval) {

    IEC61850ClientConfig* config = new IEC61850ClientConfig();

    config->importProtocolConfig(wrong_protocol_config_10);

    ASSERT_FALSE(config->m_protocolConfigComplete);
}

TEST_F(ConfigTest, ProtocolConfigNoDatasets) {

    IEC61850ClientConfig* config = new IEC61850ClientConfig();

    config->importProtocolConfig(wrong_protocol_config_11);

    ASSERT_TRUE(config->m_protocolConfigComplete);
}

TEST_F(ConfigTest, ProtocolConfigNoDynamicValue) {

    IEC61850ClientConfig* config = new IEC61850ClientConfig();

    config->importProtocolConfig(wrong_protocol_config_12);

    ASSERT_TRUE(config->m_protocolConfigComplete);
}

TEST_F(ConfigTest, ProtocolConfigReportSubscriptionsNotString) {

    IEC61850ClientConfig* config = new IEC61850ClientConfig();

    config->importProtocolConfig(wrong_protocol_config_13);

    ASSERT_TRUE(config->m_protocolConfigComplete);
}

TEST_F(ConfigTest, ProtocolConfigReportSubscriptionsNotObject) {

    IEC61850ClientConfig* config = new IEC61850ClientConfig();

    config->importProtocolConfig(wrong_protocol_config_14);

    ASSERT_TRUE(config->m_protocolConfigComplete);
}

TEST_F(ConfigTest, ProtocolConfigReportNoDataref) {

    IEC61850ClientConfig* config = new IEC61850ClientConfig();

    config->importProtocolConfig(wrong_protocol_config_15);

    ASSERT_TRUE(config->m_protocolConfigComplete);
}

TEST_F(ConfigTest, ProtocolConfigNoTrgroups) {

    IEC61850ClientConfig* config = new IEC61850ClientConfig();

    config->importProtocolConfig(wrong_protocol_config_16);

    ASSERT_TRUE(config->m_protocolConfigComplete);
}

TEST_F(ConfigTest, ProtocolConfigBuftmIntgpd) {

    IEC61850ClientConfig* config = new IEC61850ClientConfig();

    config->importProtocolConfig(wrong_protocol_config_17);

    ASSERT_TRUE(config->m_protocolConfigComplete);
}

TEST_F(ConfigTest, TestOSISelector) {
    IEC61850ClientConfig* config = new IEC61850ClientConfig();

    std::string testStr = "0x00,0x01,0x02,0x03"; 
    uint8_t selectorValue[10]; 

    ASSERT_NO_THROW({
        uint8_t parsedBytes = config->parseOsiSelector(testStr, selectorValue, sizeof(selectorValue));
        ASSERT_EQ(parsedBytes, 4); 
        for(int i = 0; i < parsedBytes; i++) {
            ASSERT_EQ(selectorValue[i], i); 
        }
    });

    testStr = "0x03"; 

    ASSERT_NO_THROW({
        uint8_t parsedBytes = config->parseOsiSelector(testStr, selectorValue, sizeof(selectorValue));
        ASSERT_EQ(parsedBytes, 1); 
        ASSERT_EQ(selectorValue[0], 3); 
    });

    testStr = "0x05,0x02"; 

    ASSERT_NO_THROW({
        uint8_t parsedBytes = config->parseOsiSelector(testStr, selectorValue, sizeof(selectorValue));
        ASSERT_EQ(parsedBytes, 2); 
        ASSERT_EQ(selectorValue[0], 5);
        ASSERT_EQ(selectorValue[1], 2);
    });

    testStr = "f143125c"; 

    ASSERT_NO_THROW({
        uint8_t parsedBytes = config->parseOsiSelector(testStr, selectorValue, sizeof(selectorValue));
        ASSERT_EQ(parsedBytes, 4);
        ASSERT_EQ(selectorValue[0],241);
        ASSERT_EQ(selectorValue[1],67);
        ASSERT_EQ(selectorValue[2],18);
        ASSERT_EQ(selectorValue[3],92);
    });

    testStr = "00000001"; 

    ASSERT_NO_THROW({
        uint8_t parsedBytes = config->parseOsiSelector(testStr, selectorValue, sizeof(selectorValue));
        ASSERT_EQ(parsedBytes, 4);
        ASSERT_EQ(selectorValue[0],0);
        ASSERT_EQ(selectorValue[1],0);
        ASSERT_EQ(selectorValue[2],0);
        ASSERT_EQ(selectorValue[3],1);
    });

    testStr = "123";
    ASSERT_THROW(config->parseOsiSelector(testStr, selectorValue, sizeof(selectorValue)), ConfigurationException);

    testStr = "0x00,0x01,0x02,0x0Z";
    ASSERT_THROW(config->parseOsiSelector(testStr, selectorValue, sizeof(selectorValue)), ConfigurationException);

    testStr = "0a0b0c0d";

    ASSERT_NO_THROW({
            uint8_t parsedBytes = config->parseOsiSelector(testStr, selectorValue, sizeof(selectorValue));
    ASSERT_EQ(parsedBytes, 4);
    ASSERT_EQ(selectorValue[0], 10);
    ASSERT_EQ(selectorValue[1], 11);
    ASSERT_EQ(selectorValue[2], 12);
    ASSERT_EQ(selectorValue[3], 13);
    });

    testStr = "ff";

    ASSERT_NO_THROW({
            uint8_t parsedBytes = config->parseOsiSelector(testStr, selectorValue, sizeof(selectorValue));
    ASSERT_EQ(parsedBytes, 1);
    ASSERT_EQ(selectorValue[0], 255);
    });

    testStr = "AaBfC112";

    ASSERT_NO_THROW({
            uint8_t parsedBytes = config->parseOsiSelector(testStr, selectorValue, sizeof(selectorValue));
    ASSERT_EQ(parsedBytes, 4);
    ASSERT_EQ(selectorValue[0], 170);
    ASSERT_EQ(selectorValue[1], 191);
    ASSERT_EQ(selectorValue[2], 193);
    ASSERT_EQ(selectorValue[3], 18);
    });

    testStr = "01A609C605CC"; 

    ASSERT_THROW({
            config->parseOsiSelector(testStr, selectorValue, 4 );
    }, ConfigurationException);

    testStr = "123G56";

    ASSERT_THROW({
            config->parseOsiSelector(testStr, selectorValue, sizeof(selectorValue));
    }, ConfigurationException);

}

TEST_F(ConfigTest, TestCommaSeparatedBytes) {
    IEC61850ClientConfig* config = new IEC61850ClientConfig();
   
    std::string testStr = "0x00,0x01,0x02,0x03"; 
    uint8_t selectorValue[4]; 

    ASSERT_NO_THROW({
        uint8_t parsedBytes = config->parseOsiSelector(testStr, selectorValue, sizeof(selectorValue));
        ASSERT_EQ(parsedBytes, 4); 
        for(int i = 0; i < parsedBytes; i++) {
            ASSERT_EQ(selectorValue[i], i); 
        }
    });


    testStr = "0x00,0x01,0xG2,0x03"; 
    ASSERT_THROW(config->parseOsiSelector(testStr, selectorValue, sizeof(selectorValue)), ConfigurationException);
}