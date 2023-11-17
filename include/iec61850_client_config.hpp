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

using namespace std;

typedef enum { GTIS, GTIM, GTIC } PIVOTROOT;
typedef enum { SPS, DPS, BSC, MV, INS, ENS, SPC, DPC, APC, INC } CDCTYPE;

typedef struct{
 string ipAddr;
  int tcpPort;
} RedGroup;

typedef struct {
    string objRef;
    CDCTYPE cdcType;
    string label;
    string id;
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
    std::vector<std::string>* entries;
    bool dynamic;
} Dataset ;


class IEC61850ClientConfig
{
public:
    IEC61850ClientConfig() {if(m_exchangeDefinitions) m_exchangeDefinitions->clear();};
    ~IEC61850ClientConfig();

    int LogLevel() {return 1;};

    void importProtocolConfig(const string& protocolConfig);
    void importExchangeConfig(const string& exchangeConfig);
    void importTlsConfig(const string& tlsConfig);

    std::vector<RedGroup*>* GetConnections(){return m_connections;};
    std::string& GetPrivateKey() {return m_privateKey;};
    std::string& GetOwnCertificate() {return m_ownCertificate;};
    std::vector<std::string>& GetRemoteCertificates() {return m_remoteCertificates;};
    std::vector<std::string>& GetCaCertificates() {return m_caCertificates;};

    static bool isValidIPAddress(const string& addrStr);

    static int getCdcTypeFromString(const std::string& cdc);

    std::unordered_map<std::string , DataExchangeDefinition*>* ExchangeDefinition() {return m_exchangeDefinitions;};

    static int GetTypeIdByName(const string& name);

    std::string* checkExchangeDataLayer(int typeId, string& objRef);

    DataExchangeDefinition* getExchangeDefinitionByLabel(const std::string& label);
    DataExchangeDefinition* getExchangeDefinitionByPivotId(const std::string& pivotId);
    DataExchangeDefinition* getExchangeDefinitionByObjRef(const std::string& objRef);

    const std::unordered_map<std::string, std::shared_ptr<ReportSubscription>>& getReportSubscriptions() const {return m_reportSubscriptions;};
    const std::unordered_map<std::string, std::shared_ptr<Dataset>>* getDatasets() const {return m_datasets;};

    long getPollingInterval(){return pollingInterval;}

private:

    static bool isMessageTypeMatching(int expectedType, int rcvdType);

    std::vector<RedGroup*>* m_connections = nullptr;

    void deleteExchangeDefinitions();

    std::unordered_map<std::string, std::shared_ptr<Dataset>>* m_datasets = nullptr;
    std::unordered_map<std::string,  DataExchangeDefinition*>* m_exchangeDefinitions = nullptr;
    std::unordered_map<std::string,  DataExchangeDefinition*>* m_exchangeDefinitionsPivotId = nullptr;
    std::unordered_map<std::string,  DataExchangeDefinition*>* m_exchangeDefinitionsObjRef = nullptr;

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
