#ifndef IEC104_CLIENT_CONNECTION_H
#define IEC104_CLIENT_CONNECTION_H

#include "iec61850_client_config.hpp"
#include <libiec61850/iec61850_client.h>
#include "datapoint.h"
#include <mutex>
#include <thread>

class IEC61850Client;

class IEC61850ClientConnection
{
public:
    IEC61850ClientConnection(IEC61850Client* client, IEC61850ClientConfig* config, const std::string& ip, int tcpPort, OsiParameters* osiParameters);
    ~IEC61850ClientConnection();

    void Start();
    void Stop();
    void Activate();

    void Disconnect();
    void Connect();

    bool Disconnected() const {return ((m_connecting == false) && (m_connected == false));};
    bool Connecting() const {return m_connecting;};
    bool Connected() const {return m_connected;};
    bool Active() const {return m_active;};

    MmsValue* readValue(IedClientError* err, const char* objRef, FunctionalConstraint fc);

    MmsValue* readDatasetValues(IedClientError* error, const char* datasetRef);

    MmsVariableSpecification* getVariableSpec(IedClientError* error, const char* objRef, FunctionalConstraint fc);

    bool operate(const std::string& objRef, DatapointValue value);

private:
    bool prepareConnection();
    IedConnection m_connection = nullptr; 
    void executePeriodicTasks();

    IEC61850Client* m_client;
    IEC61850ClientConfig* m_config;

    static void reportCallbackFunction(void* parameter, ClientReport report);

    using ConState = enum {
        CON_STATE_IDLE,
        CON_STATE_CONNECTING,
        CON_STATE_CONNECTED,
        CON_STATE_CLOSED,
        CON_STATE_WAIT_FOR_RECONNECT,
        CON_STATE_FATAL_ERROR
    };
    
    ConState m_connectionState = CON_STATE_IDLE;

    using OperationState = enum {
      CONTROL_IDLE,
      CONTROL_WAIT_FOR_SELECT,
      CONTROL_WAIT_FOR_SELECT_WITH_VALUE,
      CONTROL_SELECTED,
      CONTROL_WAIT_FOR_ACT_CON,
      CONTROL_WAIT_FOR_ACT_TERM
    };

    using ControlObjectStruct = struct{
      ControlObjectClient client;
      OperationState state;
      ControlModel mode;
      MmsValue* value;
      std::string label;
    };


    std::unordered_map<std::string, std::shared_ptr<ControlObjectStruct>> m_controlObjects;
    std::vector<std::shared_ptr<std::pair<IEC61850ClientConnection*,LinkedList>>> m_connDataSetDirectoryPairs;

    void m_initialiseControlObjects();
    void m_configDatasets();
    void m_configRcb();
    void m_setVarSpecs();
    void m_setOsiConnectionParameters();

    OsiParameters* m_osiParameters;
    int  m_tcpPort;
    std::string m_serverIp;  
    bool m_connected = false;
    bool m_active = false; 
    bool m_connecting = false;
    bool m_started = false;

    std::mutex m_conLock;
    std::mutex m_reportLock;
    
    uint64_t m_delayExpirationTime;
  
    uint64_t m_nextPollingTime = 0;
    
    std::unique_ptr<std::thread> m_conThread = nullptr;
    void _conThread();

    bool m_connect = false; 
    bool m_disconnect = false;

    static void
    controlActionHandler(uint32_t invokeId, void *parameter, IedClientError err, ControlActionType type, bool success);

    void sendActCon(const ControlObjectStruct *cos);

    void sendActTerm(const ControlObjectStruct *cos);

    static
    void commandTerminationHandler(void *parameter, ControlObjectClient connection);

    static
    void logControlErrors(ControlAddCause addCause, ControlLastApplError lastApplError, const std::string &info);
};

#endif


