#ifndef IEC61850_CLIENT_CONFIG_H
#define IEC61850_CLIENT_CONFIG_H


#include <logger.h>
#include <map>
#include <vector>
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
} DataExchangeDefinition;

class IEC61850ClientConfig
{
public:
    IEC61850ClientConfig() {if(m_exchangeDefinitions) m_exchangeDefinitions->clear();};
    //IEC61850ClientConfig(const string& protocolConfig, const string& exchangeConfig);
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

    std::map<std::string , DataExchangeDefinition*>* ExchangeDefinition() {return m_exchangeDefinitions;};

    static int GetTypeIdByName(const string& name);

    std::string* checkExchangeDataLayer(int typeId, string& objRef);

    DataExchangeDefinition* getExchangeDefinitionByLabel(std::string& label);

    long getPollingInterval(){return pollingInterval;}

private:

    static bool isMessageTypeMatching(int expectedType, int rcvdType);

    std::vector<RedGroup*>* m_connections = nullptr;

    void deleteExchangeDefinitions();

    std::map<std::string,  DataExchangeDefinition*>* m_exchangeDefinitions = nullptr;

    bool m_protocolConfigComplete = false;
    bool m_exchangeConfigComplete = false;

    std::string m_privateKey = "";
    std::string m_ownCertificate = "";
    std::vector<std::string> m_remoteCertificates;
    std::vector<std::string> m_caCertificates;

    long pollingInterval = 0;
};

#endif /* IEC61850_CLIENT_CONFIG_H */
