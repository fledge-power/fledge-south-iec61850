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


void IEC61850::setJsonConfig(const std::string& protocol_stack,
                             const std::string& exchanged_data,
                             const std::string& tls_configuration)
{
  if(m_config) delete m_config;

  m_config = new IEC61850ClientConfig();

  m_config->importProtocolConfig(protocol_stack);
  m_config->importExchangeConfig(exchanged_data);
  m_config->importTlsConfig(tls_configuration);
}

void IEC61850::start()
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

bool
IEC61850::m_spc(int count, PLUGIN_PARAMETER** params, bool withTime){
  return true;
}

