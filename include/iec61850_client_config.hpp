#ifndef IEC61850_CLIENT_CONFIG_H
#define IEC61850_CLIENT_CONFIG_H

#include "iec61850_utility.hpp"
#include "libiec61850/iec61850_client.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include <gtest/gtest.h>
#include <logger.h>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#define FRIEND_TESTS                                                          \
    FRIEND_TEST (ConnectionHandlingTest, SingleConnection);                   \
    FRIEND_TEST (ConnectionHandlingTest, SingleConnectionTLS);                \
    FRIEND_TEST (ConnectionHandlingTest, SingleConnectionReconnect);          \
    FRIEND_TEST (ControlTest, SingleCommandDirectNormal);                     \
    FRIEND_TEST (ControlTest, DoubleCommandDirectNormal);                     \
    FRIEND_TEST (ControlTest, SingleCommandDirectEnhanced);                   \
    FRIEND_TEST (ControlTest, SingleCommandSetValue);                         \
    FRIEND_TEST (ReportingTest, ReportingWithStaticDataset);                  \
    FRIEND_TEST (ReportingTest, ReportingWithDynamicDataset);                 \
    FRIEND_TEST (ReportingTest, ReportingUpdateQuality);                      \
    FRIEND_TEST (ReportingTest, ReportingGI);                                 \
    FRIEND_TEST (ReportingTest, ReportingSetpointCommand);                    \
    FRIEND_TEST (ReportingTest, ReportingChangeValueMultipleTimes);           \
    FRIEND_TEST (SpontDataTest, Polling);                                     \
    FRIEND_TEST (SpontDataTest, PollingAllCDC);                               \
    FRIEND_TEST (ControlTest, AnalogueCommandDirectNormal);                   \
    FRIEND_TEST (ControlTest, StepCommandDirectNormal);

typedef enum
{
    GTIS,
    GTIM,
    GTIC
} PIVOTROOT;

typedef enum
{
    SPS,
    DPS,
    MV,
    INS,
    ENS,
    SPC,
    DPC,
    APC,
    INC,
    BSC
} CDCTYPE;

class ConfigurationException : public std::logic_error
{
  public:
    explicit ConfigurationException (std::string const& msg)
        : std::logic_error ("Configuration exception: " + msg)
    {
    }
};

using OsiSelectorSize = uint8_t;
struct OsiParameters
{
    std::string localApTitle{ "" };
    int localAeQualifier{ 0 };
    std::string remoteApTitle{ "" };
    int remoteAeQualifier{ 0 };
    TSelector localTSelector;
    TSelector remoteTSelector;
    SSelector localSSelector;
    SSelector remoteSSelector;
    PSelector localPSelector;
    PSelector remotePSelector;
};

struct RedGroup
{
    std::string ipAddr;
    int tcpPort;
    OsiParameters osiParameters;
    bool isOsiParametersEnabled;
    bool tls;
};

struct DataExchangeDefinition
{
    std::string objRef;
    CDCTYPE cdcType;
    std::string label;
    std::string id;
    MmsVariableSpecification* spec;
};

struct ReportSubscription
{
    std::string rcbRef;
    std::string datasetRef;
    int trgops;
    int buftm;
    int intgpd;
    bool gi;
};

struct Dataset
{
    std::string datasetRef;
    std::vector<std::string> entries;
    bool dynamic;
};

class IEC61850ClientConfig
{
  public:
    IEC61850ClientConfig () { m_exchangeDefinitions.clear (); };
    ~IEC61850ClientConfig ();

    int
    LogLevel () const
    {
        return 1;
    };

    void importProtocolConfig (const std::string& protocolConfig);
    void importJsonConnectionOsiConfig (const rapidjson::Value& connOsiConfig,
                                        RedGroup& iedConnectionParam);
    void
    importJsonConnectionOsiSelectors (const rapidjson::Value& connOsiConfig,
                                      OsiParameters* osiParams);
    OsiSelectorSize parseOsiPSelector (std::string& inputOsiSelector,
                                       PSelector* pselector);
    OsiSelectorSize parseOsiSSelector (std::string& inputOsiSelector,
                                       SSelector* sselector);
    OsiSelectorSize parseOsiTSelector (std::string& inputOsiSelector,
                                       TSelector* tselector);
    OsiSelectorSize parseOsiSelector (std::string& inputOsiSelector,
                                      uint8_t* selectorValue,
                                      const uint8_t selectorSize);
    void importExchangeConfig (const std::string& exchangeConfig);
    void importTlsConfig (const std::string& tlsConfig);

    std::vector<std::shared_ptr<RedGroup> >&
    GetConnections ()
    {
        return m_connections;
    };
    std::string&
    GetPrivateKey ()
    {
        return m_privateKey;
    };
    std::string&
    GetOwnCertificate ()
    {
        return m_ownCertificate;
    };
    std::vector<std::string>&
    GetRemoteCertificates ()
    {
        return m_remoteCertificates;
    };
    std::vector<std::string>&
    GetCaCertificates ()
    {
        return m_caCertificates;
    };

    static bool isValidIPAddress (const std::string& addrStr);

    static int getCdcTypeFromString (const std::string& cdc);

    std::unordered_map<std::string, std::shared_ptr<DataExchangeDefinition> >&
    ExchangeDefinition ()
    {
        return m_exchangeDefinitions;
    };

    static int GetTypeIdByName (const std::string& name);

    std::string* checkExchangeDataLayer (int typeId, std::string& objRef);

    std::shared_ptr<DataExchangeDefinition>
    getExchangeDefinitionByLabel (const std::string& label);
    std::shared_ptr<DataExchangeDefinition>
    getExchangeDefinitionByPivotId (const std::string& pivotId);
    std::shared_ptr<DataExchangeDefinition>
    getExchangeDefinitionByObjRef (const std::string& objRef);

    const std::unordered_map<std::string,
                             std::shared_ptr<ReportSubscription> >&
    getReportSubscriptions () const
    {
        return m_reportSubscriptions;
    };
    const std::unordered_map<std::string, std::shared_ptr<Dataset> >&
    getDatasets () const
    {
        return m_datasets;
    };
    const std::unordered_map<std::string,
                             std::shared_ptr<DataExchangeDefinition> >&
    polledDatapoints () const
    {
        return m_polledDatapoints;
    };

    long
    getPollingInterval () const
    {
        return pollingInterval;
    }

  private:
    static bool isMessageTypeMatching (int expectedType, int rcvdType);

    std::vector<std::shared_ptr<RedGroup> > m_connections;

    void deleteExchangeDefinitions ();

    std::unordered_map<std::string, std::shared_ptr<DataExchangeDefinition> >
        m_polledDatapoints;
    std::unordered_map<std::string, std::shared_ptr<Dataset> > m_datasets;
    std::unordered_map<std::string, std::shared_ptr<DataExchangeDefinition> >
        m_exchangeDefinitions;
    std::unordered_map<std::string, std::shared_ptr<DataExchangeDefinition> >
        m_exchangeDefinitionsPivotId;
    std::unordered_map<std::string, std::shared_ptr<DataExchangeDefinition> >
        m_exchangeDefinitionsObjRef;

    std::unordered_map<std::string, std::shared_ptr<ReportSubscription> >
        m_reportSubscriptions;

    bool m_protocolConfigComplete = false;
    bool m_exchangeConfigComplete = false;

    std::string m_privateKey = "";
    std::string m_ownCertificate = "";
    std::vector<std::string> m_remoteCertificates;
    std::vector<std::string> m_caCertificates;

    long pollingInterval = 0;
    FRIEND_TESTS
};

#endif /* IEC61850_CLIENT_CONFIG_H */
