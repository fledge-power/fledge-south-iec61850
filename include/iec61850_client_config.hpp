#ifndef IEC61850_CLIENT_CONFIG_H
#define IEC61850_CLIENT_CONFIG_H


#include <logger.h>
#include <map>
#include <vector>
#include "iec61850_client_redgroup.hpp"

using namespace std;

typedef struct {
    string objRef;
    int typeId;
    std::string label;
} DataExchangeDefinition;

class IEC61850ClientConfig
{
public:
    IEC61850ClientConfig() {m_exchangeDefinitions.clear();};
    //IEC61850ClientConfig(const string& protocolConfig, const string& exchangeConfig);
    ~IEC61850ClientConfig();

    int LogLevel() {return 1;};

    void importProtocolConfig(const string& protocolConfig);
    void importExchangeConfig(const string& exchangeConfig);
    void importTlsConfig(const string& tlsConfig);

    std::string& GetPrivateKey() {return m_privateKey;};
    std::string& GetOwnCertificate() {return m_ownCertificate;};
    std::vector<std::string>& GetRemoteCertificates() {return m_remoteCertificates;};
    std::vector<std::string>& GetCaCertificates() {return m_caCertificates;};

    static bool isValidIPAddress(const string& addrStr);

    std::map<int, std::map<int, DataExchangeDefinition*>>& ExchangeDefinition() {return m_exchangeDefinitions;};

    static int GetTypeIdByName(const string& name);

    std::string* checkExchangeDataLayer(int typeId, string& objRef);

    DataExchangeDefinition* getExchangeDefinitionByLabel(std::string& label);

private:

    static bool isMessageTypeMatching(int expectedType, int rcvdType);

    void deleteExchangeDefinitions();

    std::map<int, std::map<int, DataExchangeDefinition*>> m_exchangeDefinitions = std::map<int, std::map<int, DataExchangeDefinition*>>();

    bool m_protocolConfigComplete = false; /* flag if protocol configuration is read */
    bool m_exchangeConfigComplete = false; /* flag if exchange configuration is read */

    std::string m_privateKey = "";
    std::string m_ownCertificate = "";
    std::vector<std::string> m_remoteCertificates;
    std::vector<std::string> m_caCertificates;
};

#endif /* IEC61850_CLIENT_CONFIG_H */
