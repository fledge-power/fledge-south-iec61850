#include <iec61850_client_config.hpp>
#include <iec61850.hpp>
#include <vector>
#include <arpa/inet.h>

#define JSON_PROTOCOL_STACK "protocol_stack"
#define JSON_TRANSPORT_LAYER "transport_layer"
#define JSON_DATASETS "datasets"
#define JSON_CONNECTIONS "connections"
#define JSON_IP "ip_addr"
#define JSON_PORT "port"
#define JSON_TLS "tls"
#define JSON_DATASET_REF "dataset_ref"
#define JSON_DATASET_ENTRIES "entries"
#define JSON_POLLING_INTERVAL "polling_interval"
#define JSON_REPORT_SUBSCRIPTIONS "report_subscriptions"
#define JSON_RCB_REF "rcb_ref"
#define JSON_TRGOPS "trgops"

#define JSON_EXCHANGED_DATA "exchanged_data"
#define JSON_DATAPOINTS "datapoints"
#define JSON_PROTOCOLS "protocols"
#define JSON_LABEL "label"

#define PROTOCOL_IEC61850 "iec61850"
#define JSON_PROT_NAME "name"
#define JSON_PROT_OBJ_REF "objref"
#define JSON_PROT_CDC "cdc"


using namespace rapidjson;


std::map<std::string,CDCTYPE> cdcMap = {
    {"SpsTyp",SPS}, {"DpsTyp",DPS},
    {"BscTyp",BSC}, {"MvTyp",MV},
    {"SpcTyp",SPC}, {"DpcTyp",DPC},
    {"ApcTyp",APC}, {"IncTyp",INC}};

int
IEC61850ClientConfig::getCdcTypeFromString( const std::string& cdc) {
  auto it = cdcMap.find(cdc);
  if (it != cdcMap.end()) {
    return it->second;
  }
  return -1;
}

DataExchangeDefinition*
IEC61850ClientConfig::getExchangeDefinitionByLabel(std::string& label){
  auto it = m_exchangeDefinitions->find(label);
  if(it !=  m_exchangeDefinitions->end()){
    return it->second;
  }
  return nullptr;
}


bool 
IEC61850ClientConfig::isValidIPAddress(const std::string& addrStr) {
  // see
  // https://stackoverflow.com/questions/318236/how-do-you-validate-that-a-string-is-a-valid-ipv4-address-in-c
  struct sockaddr_in sa;
  int result = inet_pton(AF_INET, addrStr.c_str(), &(sa.sin_addr));

  return (result == 1);
}

void 
IEC61850ClientConfig::deleteExchangeDefinitions() {
  if (!m_exchangeDefinitions) return;

  delete m_exchangeDefinitions;

  m_exchangeDefinitions = nullptr;
}

IEC61850ClientConfig::~IEC61850ClientConfig() { deleteExchangeDefinitions(); }

void 
IEC61850ClientConfig::importProtocolConfig(const std::string& protocolConfig) {
  m_protocolConfigComplete = false;

  Document document;

  if (document.Parse(const_cast<char*>(protocolConfig.c_str()))
          .HasParseError()) {
    Logger::getLogger()->fatal("Parsing error in protocol configuration");
    printf("Parsing error in protocol configuration\n");
    return;
  }

  if (!document.IsObject()) {
    return;
  }

  if (!document.HasMember(JSON_PROTOCOL_STACK) ||
      !document[JSON_PROTOCOL_STACK].IsObject()) {
    return;
  }

  const Value& protocolStack = document[JSON_PROTOCOL_STACK];

  if (!protocolStack.HasMember(JSON_TRANSPORT_LAYER) ||
      !protocolStack[JSON_TRANSPORT_LAYER].IsObject()) {
    Logger::getLogger()->fatal("transport layer configuration is missing");
    return;
  }

  const Value& transportLayer = protocolStack[JSON_TRANSPORT_LAYER];
  
  if (!transportLayer.HasMember(JSON_CONNECTIONS) ||
      !transportLayer[JSON_CONNECTIONS].IsArray()){
    LOGGER->fatal("no connections are configured");
    return;
  }
  
  const Value& connections = transportLayer[JSON_CONNECTIONS];

  m_connections = new std::vector<RedGroup*>();

  for(const Value& connection : connections.GetArray()){
   if (connection.HasMember(JSON_IP) && connection[JSON_IP].IsString()){
      std::string srvIp = connection[JSON_IP].GetString();
      if(!isValidIPAddress(srvIp)){
        LOGGER->error("Invalid Ip address %s", srvIp.c_str());
        continue;
      }
      if (connection.HasMember(JSON_PORT) && connection[JSON_PORT].IsInt()){
        int portVal = connection[JSON_PORT].GetInt();
        if(portVal <= 0 || portVal >= 65636){
          LOGGER->error("Invalid port %d", portVal);
          continue;
        }
      }

      RedGroup* group = new RedGroup;
      
      group->ipAddr = connection[JSON_IP].GetString();
      group->tcpPort = connection[JSON_PORT].GetInt();

      m_connections->push_back(group);
    }
  }
}

void
IEC61850ClientConfig::importExchangeConfig(const std::string& exchangeConfig){
  m_exchangeConfigComplete = false;

  deleteExchangeDefinitions();

  m_exchangeDefinitions = new std::map<std::string, DataExchangeDefinition*>();

  Document document;

  if (document.Parse(const_cast<char*>(exchangeConfig.c_str()))
          .HasParseError()) {
    LOGGER->fatal("Parsing error in data exchange configuration");

    return;
  }

  if (!document.IsObject()) {
    LOGGER->error("NO DOCUMENT OBJECT FOR EXCHANGED DATA");
    return;
  }
  if (!document.HasMember(JSON_EXCHANGED_DATA) ||
      !document[JSON_EXCHANGED_DATA].IsObject()) {
    LOGGER->error("EXCHANGED DATA NOT AN OBJECT");
    return;
  }

  const Value& exchangeData = document[JSON_EXCHANGED_DATA];

  if (!exchangeData.HasMember(JSON_DATAPOINTS) ||
      !exchangeData[JSON_DATAPOINTS].IsArray()) {
        LOGGER->error("NO EXCHANGED DATA DATAPOINTS");
    return;
  }

  const Value& datapoints = exchangeData[JSON_DATAPOINTS];

  for (const Value& datapoint : datapoints.GetArray()) {
    if (!datapoint.IsObject()){
      LOGGER->error("DATAPOINT NOT AN OBJECT");
      return;
    }
    
    if (!datapoint.HasMember(JSON_LABEL) || !datapoint[JSON_LABEL].IsString()){
      LOGGER->error("DATAPOINT MISSING LABEL");
      return;
    }
    std::string label = datapoint[JSON_LABEL].GetString();

    if (!datapoint.HasMember(JSON_PROTOCOLS) ||
        !datapoint[JSON_PROTOCOLS].IsArray()){
      LOGGER->error("DATAPOINT MISSING PROTOCOLS ARRAY");
      return;
    }
    for (const Value& protocol : datapoint[JSON_PROTOCOLS].GetArray()) {
      if (!protocol.HasMember(JSON_PROT_NAME) ||
          !protocol[JSON_PROT_NAME].IsString()){
        LOGGER->error("PROTOCOL MISSING NAME");
        return;
      }
      std::string protocolName = protocol[JSON_PROT_NAME].GetString();

      if (protocolName != PROTOCOL_IEC61850){
        continue;
      } 
      if (!protocol.HasMember(JSON_PROT_OBJ_REF) ||
          !protocol[JSON_PROT_OBJ_REF].IsString()){
            LOGGER->error("PROTOCOL HAS NO OBJECT REFERENCE");
        return;
      }
      if (!protocol.HasMember(JSON_PROT_CDC) ||
          !protocol[JSON_PROT_CDC].IsString()){
          LOGGER->error("PROTOCOL HAS NO CDC");
          return;
      }

      const std::string objRef = protocol[JSON_PROT_OBJ_REF].GetString();
      const std::string typeIdStr = protocol[JSON_PROT_CDC].GetString();

      LOGGER->info("  address: %s type: %s label: %s \n ", objRef.c_str(), typeIdStr.c_str(), label.c_str());
            
      int typeId = getCdcTypeFromString(typeIdStr);
      
      if(typeId == -1){
        LOGGER->error("Invalid CDC type, skip", typeIdStr.c_str());
        continue;
      }
     
      CDCTYPE cdcType = static_cast<CDCTYPE>(typeId);
      
      auto it = m_exchangeDefinitions->find(label);

      if(it!=m_exchangeDefinitions->end()){
        LOGGER->warn("DataExchangeDefinition with label %s already exists -> ignore", label.c_str());
        continue;
      }
            
      DataExchangeDefinition* def = new DataExchangeDefinition;

      def->objRef = objRef;
      def->cdcType = cdcType;
      def->label = label;

      m_exchangeDefinitions->insert({label, def});
    }
  }
}


