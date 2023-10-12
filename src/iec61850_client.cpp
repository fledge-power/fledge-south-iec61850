#include "iec61850_client_config.hpp"
#include "iec61850_client_connection.hpp"
#include <iec61850.hpp>
#include <libiec61850/iec61850_client.h>
#include <libiec61850/hal_thread.h>

static uint64_t
getMonotonicTimeInMs()
{
    uint64_t timeVal = 0;

    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        timeVal = ((uint64_t) ts.tv_sec * 1000LL) + (ts.tv_nsec / 1000000);
    }

    return timeVal;
}

IEC61850Client::IEC61850Client(IEC61850* iec61850, IEC61850ClientConfig* iec61850_client_config)
          :m_iec61850(iec61850),
           m_config(iec61850_client_config)
{}


IEC61850Client::~IEC61850Client()
{
  stop();
  delete m_connections;
}

void
IEC61850Client::stop()
{  
  if(!m_started){
    return;
  }

  m_started = false;

  if (m_monitoringThread != nullptr) {
    m_monitoringThread->join();
    delete m_monitoringThread;
    m_monitoringThread = nullptr;
  }
}

void
IEC61850Client::start(){ 
  if(m_started) return;

  prepareConnections();
  m_started = true;
  m_monitoringThread = new std::thread(&IEC61850Client::_monitoringThread, this); 
}

void
IEC61850Client::prepareConnections(){
  m_connections = new std::vector<IEC61850ClientConnection*>();
    for(RedGroup* redgroup : *m_config->GetConnections())
    {
      Logger::getLogger()->info("Add connection: %s", redgroup->ipAddr.c_str());
      IEC61850ClientConnection* connection = new IEC61850ClientConnection(this, m_config, redgroup->ipAddr, redgroup->tcpPort);

      m_connections->push_back(connection);
    }
}

void
IEC61850Client::updateConnectionStatus(ConnectionStatus newState)
{
    if (m_connStatus == newState)
      return;

    m_connStatus = newState;
}

void
IEC61850Client::_monitoringThread()
{
    uint64_t qualityUpdateTimeout = 500; /* 500 ms */
    uint64_t qualityUpdateTimer = 0;
    bool qualityUpdated = false;
    bool firstConnected = false;

    if (m_started)
    {
        for (auto clientConnection : *m_connections)
        {
            clientConnection->Start();
        }
    }

    updateConnectionStatus(ConnectionStatus::NOT_CONNECTED);

    // updateQualityForAllDataObjects(IEC60870_QUALITY_INVALID);

    uint64_t backupConnectionStartTime = Hal_getTimeInMs() + BACKUP_CONNECTION_TIMEOUT;

    while (m_started)
    {
        m_activeConnectionMtx.lock();

        if (m_active_connection == nullptr)
        {
            bool foundOpenConnections = false;

            for (auto clientConnection : *m_connections)
            {
              backupConnectionStartTime = Hal_getTimeInMs() + BACKUP_CONNECTION_TIMEOUT;

              foundOpenConnections = true;

              clientConnection->Connect();

              m_active_connection = clientConnection;

              updateConnectionStatus(ConnectionStatus::STARTED);

              break;
            }

            if (foundOpenConnections) {
                firstConnected = true;
                qualityUpdateTimer = 0;
                qualityUpdated = false;
            }

            if (foundOpenConnections == false) {

                if (firstConnected) {

                    if (qualityUpdated == false) {
                        if (qualityUpdateTimer != 0) {
                            if (getMonotonicTimeInMs() > qualityUpdateTimer) {
                                // updateQualityForAllDataObjects(IEC60870_QUALITY_NON_TOPICAL);
                                qualityUpdated = true;
                            }
                         }
                        else {
                            qualityUpdateTimer = getMonotonicTimeInMs() + qualityUpdateTimeout;
                        }
                    }

                }

                updateConnectionStatus(ConnectionStatus::NOT_CONNECTED);

                if (Hal_getTimeInMs() > backupConnectionStartTime)
                {
                    for (auto clientConnection : *m_connections)
                    {
                        if (clientConnection->Disconnected()) {
                            clientConnection->Connect();
                        }
                    }

                    backupConnectionStartTime = Hal_getTimeInMs() + BACKUP_CONNECTION_TIMEOUT;
                }
            }
        }
        else {
            backupConnectionStartTime = Hal_getTimeInMs() + BACKUP_CONNECTION_TIMEOUT;

            if (m_active_connection->Connected() == false)
            {
                m_active_connection = nullptr;
            }
        }

        m_activeConnectionMtx.unlock();

        // checkOutstandingCommandTimeouts();

        Thread_sleep(100);
    }

    for (auto clientConnection : *m_connections)
    {
        clientConnection->Stop();

        delete clientConnection;
    }

    m_connections->clear();
}
