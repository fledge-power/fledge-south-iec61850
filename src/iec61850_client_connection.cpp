#include <libiec61850/iec61850_client.h>
#include <iec61850.hpp>


IEC61850ClientConnection::IEC61850ClientConnection(IEC61850Client* client,
                                                   IEC61850ClientConfig* config,
                                                   const std::string& ip,
                                                   const int tcpPort)
{
  m_connection = IedConnection_create();
  m_client = client;
  m_config = config;
  m_serverIp = ip;
  m_tcpPort = tcpPort;
}


void
IEC61850ClientConnection::Connect(){

  IedClientError error;

  IedConnection_connect(m_connection, &error, m_serverIp.c_str(), m_tcpPort);
  IedConnection_connectAsync(m_connection, &error, m_serverIp.c_str(), m_tcpPort);

  if (error == IED_ERROR_ALREADY_CONNECTED){
    LOGGER->warn("Already connected to %s:%d", m_serverIp.c_str(), m_tcpPort);
    return;
  }

  if (error != IED_ERROR_OK) {
    LOGGER->warn("Failed to connect to %s:%d", m_serverIp.c_str(), m_tcpPort);
    return;
  }

  if(IedConnection_getState(m_connection) == IED_STATE_CONNECTED){
    LOGGER->info("Connected successfuly to %s:%d", m_serverIp.c_str(), m_tcpPort);
    m_connected = true;
  }
  
}
