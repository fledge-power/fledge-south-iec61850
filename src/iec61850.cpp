#include "iec61850_client_config.hpp"
#include "plugin_api.h"
#include <iec61850.hpp>

bool
isCommandType(CDCTYPE type){
    return type>=SPC;
}


IEC61850::~IEC61850(){
  delete m_config;
}

void
IEC61850::registerIngest(void* data, INGEST_CB cb)
{
    m_ingest = cb;
    m_data = data;
}


void
IEC61850::setJsonConfig(const std::string& protocol_stack,
                             const std::string& exchanged_data,
                             const std::string& tls_configuration)
{
  if(m_config) delete m_config;

  m_config = new IEC61850ClientConfig();

  m_config->importExchangeConfig(exchanged_data);
  m_config->importProtocolConfig(protocol_stack);
  m_config->importTlsConfig(tls_configuration);
 }

void 
IEC61850::start()
{
    Logger::getLogger()->info("Starting iec61850");

    switch (m_config->LogLevel())
    {
        case 1:
            Logger::getLogger()->setMinLevel("debug");
            break;
        case 2:
            Logger::getLogger()->setMinLevel("info");
            break;
        case 3:
            Logger::getLogger()->setMinLevel("warning");
            break;
        default:
            Logger::getLogger()->setMinLevel("error");
            break;
    }

    m_client = new IEC61850Client(this, m_config);

    if(!m_client){
        Logger::getLogger()->error("Can't start, client is null");
        return;
    }

    m_client->start();
}

void IEC61850::stop()
{
  if(!m_client) return;

  m_client->stop();

  delete m_client;
  m_client = nullptr;
}

void IEC61850::ingest(const std::string& assetName, const std::vector<Datapoint*>& points)
{
  if (m_ingest){
    m_ingest(m_data, Reading(assetName, points));
  }
}

static Datapoint*
getCdc(Datapoint* dp)
{
    DatapointValue& dpv = dp->getData();

    if (dpv.getType() != DatapointValue::T_DP_DICT) {
      Logger::getLogger()->error("Datapoint is not a dictionary %s", dp->getName().c_str());
    }

    std::vector<Datapoint*>* datapoints = dpv.getDpVec();

    for (Datapoint* child : *datapoints) {
      if(IEC61850ClientConfig::getCdcTypeFromString(child->getName()) != -1){
        return child;
      }  
    }
    
    return nullptr;
}

bool
IEC61850::operation(const std::string& operation, int count,
                       PLUGIN_PARAMETER** params)
{
    if (m_client == nullptr) {
        Logger::getLogger()->error("operation called but plugin is not yet initialized");

        return false;
    }

    if (operation == "PivotCommand"){
        std::string commandContentJSON = params[0]->value;
        
        Datapoint* commandContent;

        DatapointValue temp((long)1);
        std::unique_ptr<Datapoint> parserDp (new Datapoint("Parser", temp));
        commandContent = parserDp->parseJson(commandContentJSON)->at(0);
                
        Logger::getLogger()->debug("Received command: %s", commandContent->toJSONProperty().c_str());
        
        Datapoint* cdc = getCdc(commandContent);
        
        if(!cdc){
          Logger::getLogger()->warn("Received pivot object has no cdc");
          return false;
        }

        int type = IEC61850ClientConfig::getCdcTypeFromString(cdc->getName());

        if(type == -1 || !isCommandType((CDCTYPE)type)){
          Logger::getLogger()->warn("Not a command object %s -> ignore", cdc->toJSONProperty().c_str());
          return false;
        }

        return m_client->handleOperation(commandContent);
     }

    Logger::getLogger()->error("Unrecognised operation %s", operation.c_str());

    return false;
}

