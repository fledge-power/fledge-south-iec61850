#include <iec61850_client_config.hpp>
#include <iec61850.hpp>
#include <vector>
#include <arpa/inet.h>

#define JSON_PROTOCOL_STACK "protocol_stack"
#define JSON_TRANSPORT_LAYER "transport_layer"
#define JSON_APPLICATION_LAYER "application_layer"
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
#define JSON_PIVOT_ID "pivot_id"

#define PROTOCOL_IEC61850 "iec61850"
#define JSON_PROT_NAME "name"
#define JSON_PROT_OBJ_REF "objref"
#define JSON_PROT_CDC "cdc"


using namespace rapidjson;

static
std::map<std::string, int> trgOptions = {
        {"data_changed", TRG_OPT_DATA_CHANGED},
        {"quality_changed", TRG_OPT_QUALITY_CHANGED},
        {"data_update", TRG_OPT_DATA_UPDATE},
        {"integrity", TRG_OPT_INTEGRITY},
        {"gi", TRG_OPT_GI},
        {"transient", TRG_OPT_TRANSIENT}
};

static
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
IEC61850ClientConfig::getExchangeDefinitionByLabel(const std::string& label){
  auto it = m_exchangeDefinitions->find(label);
  if(it !=  m_exchangeDefinitions->end()){
    return it->second;
  }
  return nullptr;
}

DataExchangeDefinition*
IEC61850ClientConfig::getExchangeDefinitionByPivotId(const std::string &pivotId) {
    auto it = m_exchangeDefinitionsPivotId->find(pivotId);
    if(it !=  m_exchangeDefinitionsPivotId->end()){
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

  if (!m_exchangeDefinitionsPivotId) return;

  delete m_exchangeDefinitionsPivotId;

  m_exchangeDefinitionsPivotId = nullptr;

    if (!m_exchangeDefinitionsObjRef) return;

    delete m_exchangeDefinitionsObjRef;

    m_exchangeDefinitionsObjRef = nullptr;
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
    Logger::getLogger()->fatal("no connections are configured");
    return;
  }
  
  const Value& connections = transportLayer[JSON_CONNECTIONS];

  m_connections = new std::vector<RedGroup*>();

  for(const Value& connection : connections.GetArray()){
   if (connection.HasMember(JSON_IP) && connection[JSON_IP].IsString()){
      std::string srvIp = connection[JSON_IP].GetString();
      if(!isValidIPAddress(srvIp)){
        Logger::getLogger()->error("Invalid Ip address %s", srvIp.c_str());
        continue;
      }
      if (connection.HasMember(JSON_PORT) && connection[JSON_PORT].IsInt()){
        int portVal = connection[JSON_PORT].GetInt();
        if(portVal <= 0 || portVal >= 65636){
          Logger::getLogger()->error("Invalid port %d", portVal);
          continue;
        }
      }

      RedGroup* group = new RedGroup;
      
      group->ipAddr = connection[JSON_IP].GetString();
      group->tcpPort = connection[JSON_PORT].GetInt();

      m_connections->push_back(group);
    }
  }
  
  if (!protocolStack.HasMember(JSON_APPLICATION_LAYER) ||
    !protocolStack[JSON_APPLICATION_LAYER].IsObject()) {
    Logger::getLogger()->fatal("transport layer configuration is missing");
    return;
  }
   
  const Value& applicationLayer = protocolStack[JSON_APPLICATION_LAYER];

  if(applicationLayer.HasMember(JSON_POLLING_INTERVAL)){
    if(!applicationLayer[JSON_POLLING_INTERVAL].IsInt()){
      Logger::getLogger()->error("polling_interval has invalid data type");
      return;
    }
    int intVal = applicationLayer[JSON_POLLING_INTERVAL].GetInt();
    if(intVal < 0){
      Logger::getLogger()->error("polling_interval must be positive");
      return;
    }
    pollingInterval = intVal;
  }

  if (applicationLayer.HasMember(JSON_DATASETS) && applicationLayer[JSON_DATASETS].IsArray()) {
      m_datasets = new std::unordered_map<std::string, std::shared_ptr<Dataset>>();
      for (const auto& datasetVal : applicationLayer[JSON_DATASETS].GetArray()) {
          if (!datasetVal.IsObject() || !datasetVal.HasMember(JSON_DATASET_REF) || !datasetVal[JSON_DATASET_REF].IsString()) continue;

          std::string datasetRef = datasetVal[JSON_DATASET_REF].GetString();
          auto dataset = std::make_shared<Dataset>();
          dataset->entries = nullptr;
          if (datasetVal.HasMember(JSON_DATASET_ENTRIES) && datasetVal[JSON_DATASET_ENTRIES].IsArray()) {
              dataset->entries = new std::vector<DataExchangeDefinition*>();
              for (const auto& entryVal : datasetVal[JSON_DATASET_ENTRIES].GetArray()) {
                  if (entryVal.IsString()) {
                      std::string objref = entryVal.GetString();
                      DataExchangeDefinition* def = getExchangeDefinitionByObjRef(objref);
                      if (def) {
                          Logger::getLogger()->debug("Add entry %s to dataset %s", objref.c_str(), datasetRef.c_str());
                          dataset->entries->push_back(def);
                      }
                  }
              }
          }
          m_datasets->insert({datasetRef, dataset});
      }
    }

    if (!applicationLayer.HasMember(JSON_REPORT_SUBSCRIPTIONS) ||
        !applicationLayer[JSON_REPORT_SUBSCRIPTIONS].IsArray()) {
        Logger::getLogger()->error("No report subscriptions are configured");
        return;
    }

    for (const auto& reportVal : applicationLayer[JSON_REPORT_SUBSCRIPTIONS].GetArray()) {
        if (!reportVal.IsObject()) continue;
        auto report = std::make_shared<ReportSubscription>();

        if (reportVal.HasMember(JSON_RCB_REF) && reportVal[JSON_RCB_REF].IsString()) {
            report->rcbRef = reportVal[JSON_RCB_REF].GetString();
        }
        else{
            continue;
        }

        if (reportVal.HasMember(JSON_DATASET_REF) && reportVal[JSON_DATASET_REF].IsString()) {
            report->datasetRef = reportVal[JSON_DATASET_REF].GetString();
        }
        else{
            report->datasetRef = "";
        }

        if (reportVal.HasMember(JSON_TRGOPS) && reportVal[JSON_TRGOPS].IsArray()) {
            for (const auto& trgopVal : reportVal[JSON_TRGOPS].GetArray()) {
                if (trgopVal.IsString()) {
                    auto it = trgOptions.find(trgopVal.GetString());
                    if(it == trgOptions.end()) continue;
                    report->trgops |= it->second;
                }
            }
        }
        else{
            report->trgops = -1;
        }

        if (reportVal.HasMember("buftm") && reportVal["buftm"].IsInt()) {
            report->buftm = reportVal["buftm"].GetInt();
        }
        else{
            report->buftm = -1;
        }

        if (reportVal.HasMember("intgpd") && reportVal["intgpd"].IsInt()) {
            report->intgpd = reportVal["intgpd"].GetInt();
        }
        else{
            report->intgpd = -1;
        }

        m_reportSubscriptions.insert({report->rcbRef, std::move(report)});
    }

}

void
IEC61850ClientConfig::importExchangeConfig(const std::string& exchangeConfig){
  m_exchangeConfigComplete = false;

  deleteExchangeDefinitions();

  m_exchangeDefinitions = new std::unordered_map<std::string, DataExchangeDefinition*>();
  m_exchangeDefinitionsPivotId = new std::unordered_map<std::string, DataExchangeDefinition*>();
  m_exchangeDefinitionsObjRef = new std::unordered_map<std::string, DataExchangeDefinition*>();

  Document document;

  if (document.Parse(const_cast<char*>(exchangeConfig.c_str()))
          .HasParseError()) {
    Logger::getLogger()->fatal("Parsing error in data exchange configuration");

    return;
  }

  if (!document.IsObject()) {
    Logger::getLogger()->error("NO DOCUMENT OBJECT FOR EXCHANGED DATA");
    return;
  }
  if (!document.HasMember(JSON_EXCHANGED_DATA) ||
      !document[JSON_EXCHANGED_DATA].IsObject()) {
    Logger::getLogger()->error("EXCHANGED DATA NOT AN OBJECT");
    return;
  }

  const Value& exchangeData = document[JSON_EXCHANGED_DATA];

  if (!exchangeData.HasMember(JSON_DATAPOINTS) ||
      !exchangeData[JSON_DATAPOINTS].IsArray()) {
        Logger::getLogger()->error("NO EXCHANGED DATA DATAPOINTS");
    return;
  }

  const Value& datapoints = exchangeData[JSON_DATAPOINTS];

  for (const Value& datapoint : datapoints.GetArray()) {
    if (!datapoint.IsObject()){
      Logger::getLogger()->error("DATAPOINT NOT AN OBJECT");
      return;
    }
    
    if (!datapoint.HasMember(JSON_LABEL) || !datapoint[JSON_LABEL].IsString()){
      Logger::getLogger()->error("DATAPOINT MISSING LABEL");
      return;
    }
    std::string label = datapoint[JSON_LABEL].GetString();

    if (!datapoint.HasMember(JSON_PIVOT_ID) || !datapoint[JSON_PIVOT_ID].IsString()){
        Logger::getLogger()->error("DATAPOINT MISSING PIVOT ID");
        return;
    }

    std::string pivot_id = datapoint[JSON_PIVOT_ID].GetString();

    if (!datapoint.HasMember(JSON_PROTOCOLS) ||
        !datapoint[JSON_PROTOCOLS].IsArray()){
      Logger::getLogger()->error("DATAPOINT MISSING PROTOCOLS ARRAY");
      return;
    }
    for (const Value& protocol : datapoint[JSON_PROTOCOLS].GetArray()) {
      if (!protocol.HasMember(JSON_PROT_NAME) ||
          !protocol[JSON_PROT_NAME].IsString()){
        Logger::getLogger()->error("PROTOCOL MISSING NAME");
        return;
      }
      std::string protocolName = protocol[JSON_PROT_NAME].GetString();

      if (protocolName != PROTOCOL_IEC61850){
        continue;
      } 
      if (!protocol.HasMember(JSON_PROT_OBJ_REF) ||
          !protocol[JSON_PROT_OBJ_REF].IsString()){
            Logger::getLogger()->error("PROTOCOL HAS NO OBJECT REFERENCE");
        return;
      }
      if (!protocol.HasMember(JSON_PROT_CDC) ||
          !protocol[JSON_PROT_CDC].IsString()){
          Logger::getLogger()->error("PROTOCOL HAS NO CDC");
          return;
      }

      const std::string objRef = protocol[JSON_PROT_OBJ_REF].GetString();
      const std::string typeIdStr = protocol[JSON_PROT_CDC].GetString();

      Logger::getLogger()->info("  address: %s type: %s label: %s \n ", objRef.c_str(), typeIdStr.c_str(), label.c_str());
            
      int typeId = getCdcTypeFromString(typeIdStr);
      
      if(typeId == -1){
        Logger::getLogger()->error("Invalid CDC type, skip", typeIdStr.c_str());
        continue;
      }
     
      CDCTYPE cdcType = static_cast<CDCTYPE>(typeId);
      
      auto it = m_exchangeDefinitions->find(label);

      if(it!=m_exchangeDefinitions->end()){
        Logger::getLogger()->warn("DataExchangeDefinition with label %s already exists -> ignore", label.c_str());
        continue;
      }
            
      DataExchangeDefinition* def = new DataExchangeDefinition;

      def->objRef = objRef;
      def->cdcType = cdcType;
      def->label = label;
      def->id = pivot_id;

      m_exchangeDefinitions->insert({label, def});
      m_exchangeDefinitionsPivotId->insert({pivot_id, def});
      m_exchangeDefinitionsObjRef->insert({objRef, def});
    }
  }
}

DataExchangeDefinition*
IEC61850ClientConfig::getExchangeDefinitionByObjRef(const std::string &objRef) {
    auto it = m_exchangeDefinitionsObjRef->find(objRef);
    if(it !=  m_exchangeDefinitionsObjRef->end()){
        return it->second;
    }
    return nullptr;
}


