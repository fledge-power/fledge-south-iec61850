#ifndef IEC61850_CLIENT_CONFIG_H
#define IEC61850_CLIENT_CONFIG_H

#include "libiec61850/iec61850_client.h"

#include <logger.h>
#include <map>
#include <vector>
#include <unordered_map>
#include <memory>
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

typedef enum { GTIS, GTIM, GTIC } PIVOTROOT;
typedef enum { SPS, DPS, BSC, MV, INS, ENS, SPC, DPC, APC, INC } CDCTYPE;

typedef struct{
  std::string ipAddr;
  int tcpPort;
} RedGroup;

typedef struct {
    std::string objRef;
    CDCTYPE cdcType;
    std::string label;
    std::string id;
    MmsVariableSpecification* spec;
} DataExchangeDefinition;

typedef struct{
    std::string rcbRef;
    std::string datasetRef;
    int trgops;
    int buftm;
    int intgpd;
    bool gi;
} ReportSubscription ;

typedef struct{
    std::string datasetRef;
    std::vector<std::string> entries;
    bool dynamic;
} Dataset ;


class IEC61850ClientConfig
{
public:
    IEC61850ClientConfig() {m_exchangeDefinitions.clear();};
    ~IEC61850ClientConfig();

    int LogLevel() const{return 1;};

    void importProtocolConfig(const std::string& protocolConfig);
    void importExchangeConfig(const std::string& exchangeConfig);
    void importTlsConfig(const std::string& tlsConfig);

    std::vector<std::shared_ptr<RedGroup>>& GetConnections(){return m_connections;};
    std::string& GetPrivateKey() {return m_privateKey;};
    std::string& GetOwnCertificate() {return m_ownCertificate;};
    std::vector<std::string>& GetRemoteCertificates() {return m_remoteCertificates;};
    std::vector<std::string>& GetCaCertificates() {return m_caCertificates;};

    static bool isValidIPAddress(const std::string& addrStr);

    static int getCdcTypeFromString(const std::string& cdc);

    std::unordered_map<std::string , std::shared_ptr<DataExchangeDefinition>>& ExchangeDefinition() {return m_exchangeDefinitions;};

    static int GetTypeIdByName(const std::string& name);

    std::string* checkExchangeDataLayer(int typeId, std::string& objRef);

    std::shared_ptr<DataExchangeDefinition> getExchangeDefinitionByLabel(const std::string& label);
    std::shared_ptr<DataExchangeDefinition> getExchangeDefinitionByPivotId(const std::string& pivotId);
    std::shared_ptr<DataExchangeDefinition> getExchangeDefinitionByObjRef(const std::string& objRef);

    const std::unordered_map<std::string, std::shared_ptr<ReportSubscription>>& getReportSubscriptions() const {return m_reportSubscriptions;};
    const std::unordered_map<std::string, std::shared_ptr<Dataset>>& getDatasets() const {return m_datasets;};

    long getPollingInterval() const{return pollingInterval;}

private:

    static bool isMessageTypeMatching(int expectedType, int rcvdType);

    std::vector<std::shared_ptr<RedGroup>> m_connections;

    void deleteExchangeDefinitions();

    std::unordered_map<std::string, std::shared_ptr<Dataset>> m_datasets;
    std::unordered_map<std::string,  std::shared_ptr<DataExchangeDefinition>> m_exchangeDefinitions;
    std::unordered_map<std::string,  std::shared_ptr<DataExchangeDefinition>> m_exchangeDefinitionsPivotId;
    std::unordered_map<std::string,  std::shared_ptr<DataExchangeDefinition>> m_exchangeDefinitionsObjRef;

    std::unordered_map<std::string, std::shared_ptr<ReportSubscription>> m_reportSubscriptions;

    bool m_protocolConfigComplete = false;
    bool m_exchangeConfigComplete = false;

    std::string m_privateKey = "";
    std::string m_ownCertificate = "";
    std::vector<std::string> m_remoteCertificates;
    std::vector<std::string> m_caCertificates;

    long pollingInterval = 0;
};

#endif /* IEC61850_CLIENT_CONFIG_H */
