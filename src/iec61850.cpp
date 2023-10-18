#include "iec61850_client_config.hpp"
#include "plugin_api.h"
#include <iec61850.hpp>


IEC61850::IEC61850() : m_client(nullptr)
{
  m_config = new IEC61850ClientConfig();
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

  m_config->importProtocolConfig(protocol_stack);
  m_config->importExchangeConfig(exchanged_data);
//   m_config->importTlsConfig(tls_configuration);
 }

void 
IEC61850::start()
{
    LOGGER->info("Starting iec61850");

    switch (m_config->LogLevel())
    {
        case 1:
            LOGGER->setMinLevel("debug");
            break;
        case 2:
            LOGGER->setMinLevel("info");
            break;
        case 3:
            LOGGER->setMinLevel("warning");
            break;
        default:
            LOGGER->setMinLevel("error");
            break;
    }

    m_client = new IEC61850Client(this, m_config);

    m_client->start();
}

void IEC61850::stop()
{
  if(!m_client) return;

  m_client->stop();

  delete m_client;
  m_client = nullptr;
}

void IEC61850::ingest(std::string assetName, std::vector<Datapoint*>& points)
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
      LOGGER->error("Datapoint is not a dictionary %s", dp->getName().c_str());
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
IEC61850::m_spc(Datapoint* cdc){
  
  return true;
}

bool
IEC61850::m_dpc(Datapoint* cdc){
  return true;
}

bool
IEC61850::m_apc(Datapoint* cdc){
  return true;
}

bool
IEC61850::m_inc(Datapoint* cdc){
  return true; 
}

bool
IEC61850::m_bsc(Datapoint* cdc){
  return true;
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

        commandContent = commandContent->parseJson(commandContentJSON)->at(0);
        
        LOGGER->debug("Received command: %s", commandContent->toJSONProperty().c_str());
        
        Datapoint* cdc = getCdc(commandContent);
        
        CDCTYPE type = (CDCTYPE) IEC61850ClientConfig::getCdcTypeFromString(cdc->getName());

        switch(type){
          case SPC: return m_spc(cdc);
          case DPC: return m_dpc(cdc);
          case APC: return m_apc(cdc);
          case INC: return m_inc(cdc);
          case BSC: return m_bsc(cdc);
          default:{
            LOGGER->error("Invalid command CDC");
            return false;
          }
        }
         
         
        // int typeID = m_config->GetTypeIdByName(type);
        //
        // switch (typeID){
        //     default:
        //         Logger::getLogger()->error("Unrecognised command type %s", type.c_str());
        //         return false;
        // }
    }

    Logger::getLogger()->error("Unrecognised operation %s", operation.c_str());

    return false;
}

