#include "iec61850_client_connection.hpp"
#include <libiec61850/iec61850_client.h>
#include <iec61850.hpp>
#include <libiec61850/hal_thread.h>
#include <libiec61850/iec61850_common.h>
#include <libiec61850/mms_value.h>

IEC61850ClientConnection::IEC61850ClientConnection(IEC61850Client* client,
                                                   IEC61850ClientConfig* config,
                                                   const std::string& ip,
                                                   const int tcpPort)
{
  m_client = client;
  m_config = config;
  m_serverIp = ip;
  m_tcpPort = tcpPort;
}

IEC61850ClientConnection::~IEC61850ClientConnection()
{
  Stop();  
}

static uint64_t getMonotonicTimeInMs()
{
    uint64_t timeVal = 0;

    struct timespec ts;
    
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        timeVal = ((uint64_t) ts.tv_sec * 1000LL) + (ts.tv_nsec / 1000000);
    }

    return timeVal;
}

void
IEC61850ClientConnection::Start()
{
    if (m_started == false)
    {
        m_connect = true;

        m_started = true;

        m_conThread = new std::thread(&IEC61850ClientConnection::_conThread, this);
    }
}

void
IEC61850ClientConnection::Stop()
{
    if (m_started == false) return;
    
    m_started = false;
    
    if(!m_conThread) return;
    
    m_conThread->join();
    delete m_conThread;
    m_conThread = nullptr;
}

bool
IEC61850ClientConnection::prepareConnection(){
  m_connection = IedConnection_create();
  return m_connection!=nullptr;
}

void
IEC61850ClientConnection::Disconnect()
{
    m_connect = false;
}

void
IEC61850ClientConnection::Connect()
{
    m_connect = true;
}

MmsVariableSpecification*
IEC61850ClientConnection::getVariableSpec(IedClientError* error, const char* objRef)
{
  return IedConnection_getVariableSpecification(m_connection, error, objRef, IEC61850_FC_ST);
}

MmsValue* 
IEC61850ClientConnection::readValue(IedClientError* error, const char* objRef)
{
  MmsValue* value = IedConnection_readObject(m_connection, error, objRef, IEC61850_FC_ST); 
  return value;
}

void
IEC61850ClientConnection::executePeriodicTasks()
{
  uint64_t currentTime = getMonotonicTimeInMs();
  if(m_config->getPollingInterval()>0){
    if(currentTime > m_nextPollingTime){
      m_client->handleValues();
      m_nextPollingTime = currentTime + m_config->getPollingInterval();
    }
  }
}

void
IEC61850ClientConnection::_conThread()
{
    while (m_started)
    {
        switch (m_connectionState) {

            case CON_STATE_IDLE:
                if (m_connect) {

                    IedConnection con = nullptr;

                    m_conLock.lock();

                    con = m_connection;

                    m_connection = nullptr;

                    m_conLock.unlock();

                    if (con != nullptr) {
                        IedConnection_destroy(con);
                    }

                    m_conLock.lock();

                    if (prepareConnection()) {
                        
                        IedClientError error;

                        m_connectionState = CON_STATE_CONNECTING;
                        m_connecting = true;

                        m_delayExpirationTime = getMonotonicTimeInMs() + 10000;

                        m_conLock.unlock();

                        IedConnection_connectAsync(m_connection,&error, m_serverIp.c_str(),m_tcpPort); 
                        
                        if(error == IED_ERROR_OK){
                            LOGGER->info("Connecting to %s:%d", m_serverIp.c_str(), m_tcpPort);
                            m_connectionState = CON_STATE_CONNECTING;
                        }
                        
                        else{
                          LOGGER->error("Failed to connect to %s:%d", m_serverIp.c_str(), m_tcpPort);
                        }
                    }
                    else {
                        m_connectionState = CON_STATE_FATAL_ERROR;
                        LOGGER->error("Fatal configuration error");

                        m_conLock.unlock();
                    }

                }

                break;

            case CON_STATE_CONNECTING:{
                /* wait for connected event or timeout */
                IedConnectionState newState = IedConnection_getState(m_connection);
                if(newState == IED_STATE_CONNECTED){
                  LOGGER->info("Connected to %s:%d", m_serverIp.c_str(), m_tcpPort);
                  m_connectionState = CON_STATE_CONNECTED;
                }

                else if (getMonotonicTimeInMs() > m_delayExpirationTime) {
                    LOGGER->warn("Timeout while connecting");
                    m_connectionState = CON_STATE_IDLE;
                }

                break;
            }
            case CON_STATE_CONNECTED:{

                IedConnectionState newState = IedConnection_getState(m_connection);
                
                if(newState != IED_STATE_CONNECTED){
                  m_connectionState = CON_STATE_IDLE;
                  break;
                }
                executePeriodicTasks();
                
                break;
            }
            case CON_STATE_CLOSED:

                m_delayExpirationTime = getMonotonicTimeInMs() + 10000;
                m_connectionState = CON_STATE_WAIT_FOR_RECONNECT;

                break;

            case CON_STATE_WAIT_FOR_RECONNECT:

                if (getMonotonicTimeInMs() >= m_delayExpirationTime) {
                    m_connectionState = CON_STATE_IDLE;
                }

                break;

            case CON_STATE_FATAL_ERROR:
                /* stay in this state until stop is called */
                break;
        }

        if (!m_connect)
        {
            IedConnection con = nullptr;

            m_conLock.lock();

            m_connected = false;
            m_connecting = false;
            m_disconnect = false;

            con = m_connection;

            m_connection = nullptr;

            m_conLock.unlock();

            if (con) {
                IedConnection_destroy(con);
            }
        }

        Thread_sleep(50);
    }


    IedConnection con = nullptr;

    m_conLock.lock();

    con = m_connection;

    m_connection = nullptr;

    m_conLock.unlock();

    if (con) {
        IedConnection_destroy(con);
    }
  }


