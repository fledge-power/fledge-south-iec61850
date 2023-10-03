#include "iec61850_client_config.hpp"
#include "iec61850_client_connection.hpp"
#include <iec61850.hpp>
#include <libiec61850/iec61850_client.h>

IEC61850Client::IEC61850Client(IEC61850* iec61850, IEC61850ClientConfig* iec61850_client_config)
          :m_iec61850(iec61850),
           m_config(iec61850_client_config)
{}


IEC61850Client::~IEC61850Client()
{
  stop();
}

void
IEC61850Client::stop()
{  
  if(!m_started) 
  {
    return;
  }

  m_started = false;
}

void
IEC61850Client::start(){ 
  m_connections = new std::vector<IEC61850ClientConnection*>();
  
  for(RedGroup* redgroup : *m_config->GetConnections())
  {
    IEC61850ClientConnection* connection = new IEC61850ClientConnection(this, m_config, redgroup->ipAddr, redgroup->tcpPort);

    m_connections->push_back(connection);
  }

  m_connections->at(0)->Connect();
}

