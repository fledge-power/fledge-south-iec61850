#include "iec61850_client_connection.hpp"
#include "iec61850_client_config.hpp"
#include <libiec61850/iec61850_client.h>
#include <iec61850.hpp>
#include <libiec61850/hal_thread.h>
#include <libiec61850/mms_value.h>
#include <map>
#include <string>

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
IEC61850ClientConnection::commandTerminationHandler(void *parameter, ControlObjectClient connection)
{
    LastApplError lastApplError = ControlObjectClient_getLastApplError(connection);
    if (lastApplError.error != CONTROL_ERROR_NO_ERROR) {
        logControlErrors(lastApplError.addCause, lastApplError.error, std::string(ControlObjectClient_getObjectReference(connection)));
        Logger::getLogger()->error("Couldn't terminate command");
        return;
    }

    auto connectionCosPair = (std::pair<IEC61850ClientConnection*,ControlObjectStruct*>*) parameter;
    IEC61850ClientConnection* con = connectionCosPair->first;
    ControlObjectStruct* cos = connectionCosPair->second;

    cos->state = CONTROL_IDLE;
    con->sendActTerm(cos);
    delete connectionCosPair;
}

void IEC61850ClientConnection::logControlErrors(ControlAddCause addCause, ControlLastApplError lastApplError, const std::string& info)
{
    Logger::getLogger()->error("In here : %s", info.c_str());
    switch (addCause) {
        case ADD_CAUSE_UNKNOWN:
            Logger::getLogger()->error("Unknown add cause");
            break;
        case ADD_CAUSE_NOT_SUPPORTED:
            Logger::getLogger()->error("Add cause not supported");
            break;
        case ADD_CAUSE_BLOCKED_BY_SWITCHING_HIERARCHY:
            Logger::getLogger()->error("Blocked by switching hierarchy");
            break;
        case ADD_CAUSE_SELECT_FAILED:
            Logger::getLogger()->error("Select failed");
            break;
        case ADD_CAUSE_INVALID_POSITION:
            Logger::getLogger()->error("Invalid position");
            break;
        case ADD_CAUSE_POSITION_REACHED:
            Logger::getLogger()->error("Position reached");
            break;
        case ADD_CAUSE_PARAMETER_CHANGE_IN_EXECUTION:
            Logger::getLogger()->error("Parameter change in execution");
            break;
        case ADD_CAUSE_STEP_LIMIT:
            Logger::getLogger()->error("Step limit reached");
            break;
        case ADD_CAUSE_BLOCKED_BY_MODE:
            Logger::getLogger()->error("Blocked by mode");
            break;
        case ADD_CAUSE_BLOCKED_BY_PROCESS:
            Logger::getLogger()->error("Blocked by process");
            break;
        case ADD_CAUSE_BLOCKED_BY_INTERLOCKING:
            Logger::getLogger()->error("Blocked by interlocking");
            break;
        case ADD_CAUSE_BLOCKED_BY_SYNCHROCHECK:
            Logger::getLogger()->error("Blocked by synchrocheck");
            break;
        case ADD_CAUSE_COMMAND_ALREADY_IN_EXECUTION:
            Logger::getLogger()->error("Command already in execution");
            break;
        case ADD_CAUSE_BLOCKED_BY_HEALTH:
            Logger::getLogger()->error("Blocked by health status");
            break;
        case ADD_CAUSE_1_OF_N_CONTROL:
            Logger::getLogger()->error("1 of N control error");
            break;
        case ADD_CAUSE_ABORTION_BY_CANCEL:
            Logger::getLogger()->error("Aborted by cancel");
            break;
        case ADD_CAUSE_TIME_LIMIT_OVER:
            Logger::getLogger()->error("Time limit exceeded");
            break;
        case ADD_CAUSE_ABORTION_BY_TRIP:
            Logger::getLogger()->error("Aborted by trip");
            break;
        case ADD_CAUSE_OBJECT_NOT_SELECTED:
            Logger::getLogger()->error("Object not selected");
            break;
        case ADD_CAUSE_OBJECT_ALREADY_SELECTED:
            Logger::getLogger()->error("Object already selected");
            break;
        case ADD_CAUSE_NO_ACCESS_AUTHORITY:
            Logger::getLogger()->error("No access authority");
            break;
        case ADD_CAUSE_ENDED_WITH_OVERSHOOT:
            Logger::getLogger()->error("Ended with overshoot");
            break;
        case ADD_CAUSE_ABORTION_DUE_TO_DEVIATION:
            Logger::getLogger()->error("Aborted due to deviation");
            break;
        case ADD_CAUSE_ABORTION_BY_COMMUNICATION_LOSS:
            Logger::getLogger()->error("Aborted by communication loss");
            break;
        case ADD_CAUSE_ABORTION_BY_COMMAND:
            Logger::getLogger()->error("Aborted by command");
            break;
        case ADD_CAUSE_NONE:
            Logger::getLogger()->info("No add cause error");
            break;
        case ADD_CAUSE_INCONSISTENT_PARAMETERS:
            Logger::getLogger()->error("Inconsistent parameters");
            break;
        case ADD_CAUSE_LOCKED_BY_OTHER_CLIENT:
            Logger::getLogger()->error("Locked by another client");
            break;
    }

    switch (lastApplError) {
        case CONTROL_ERROR_NO_ERROR:
            Logger::getLogger()->info("No last application error");
            break;
        case CONTROL_ERROR_UNKNOWN:
            Logger::getLogger()->error("Unknown last application error");
            break;
        case CONTROL_ERROR_TIMEOUT_TEST:
            Logger::getLogger()->error("Timeout test error");
            break;
        case CONTROL_ERROR_OPERATOR_TEST:
            Logger::getLogger()->error("Operator test error");
            break;
    }
}


void
IEC61850ClientConnection::m_initialiseControlObjects()
{
  m_controlObjects = new std::map<std::string, ControlObjectStruct*>();

  for(const auto& entry: *m_config->ExchangeDefinition()){
    DataExchangeDefinition* def = entry.second;
    if(def->cdcType<SPC) continue;
    auto* co = new ControlObjectStruct;
    IedClientError err;
    IedConnection_readObject(m_connection,&err,def->objRef.c_str(),IEC61850_FC_ST);
    if(err != IED_ERROR_OK){
        m_client->logIedClientError(err, "Initialise control object");
        continue;
    }
    co->client = ControlObjectClient_create(def->objRef.c_str(),m_connection);
    co->mode = ControlObjectClient_getControlModel(co->client);
    co->state = CONTROL_IDLE;
    co->label = entry.first;
    switch(def->cdcType) {
        case SPC: {
            co->value = MmsValue_newBoolean(false);
            break;
        }
        case DPC: {
            co->value = MmsValue_newBitString(2);
            break;
        }
        case APC: {
            co->value = MmsValue_newFloat(0.0);
            break;
        }
        case BSC:
        case INC: {
            co->value = MmsValue_newIntegerFromInt32(0);
            break;
        }
        default: {
            Logger::getLogger()->error("Invalid cdc type");
            return;
        }
    }
    Logger::getLogger()->debug("Added control object %s , %s ", co->label.c_str(), def->objRef.c_str());
    m_controlObjects->insert({def->objRef,co});
  }

}

void
IEC61850ClientConnection::Start()
{
    if (!m_started)
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
IEC61850ClientConnection::controlActionHandler(uint32_t invokeId, void* parameter, IedClientError err, ControlActionType type, bool success)
{
    if(success){
        auto connectionCosPair = (std::pair<IEC61850ClientConnection*,ControlObjectStruct*>*) parameter;

        ControlObjectStruct* cos = connectionCosPair->second;

        IEC61850ClientConnection* connection = connectionCosPair->first;
        switch(type) {
            case CONTROL_ACTION_TYPE_OPERATE: {
                if (cos->mode == CONTROL_MODEL_SBO_ENHANCED || cos->mode == CONTROL_MODEL_DIRECT_ENHANCED) {
                    cos->state = CONTROL_WAIT_FOR_ACT_TERM;
                } else {
                    cos->state = CONTROL_IDLE;
                    delete connectionCosPair;
                }
                connection->sendActCon(cos);
                break;
            }
            case CONTROL_ACTION_TYPE_SELECT:{
                cos->state = CONTROL_SELECTED;
                break;
            }
        }
    }
}

void
IEC61850ClientConnection::executePeriodicTasks()
{
  uint64_t currentTime = getMonotonicTimeInMs();
  if(m_config->getPollingInterval()>0){
    if(currentTime >= m_nextPollingTime){
      m_client->handleValues();
      m_nextPollingTime = currentTime + m_config->getPollingInterval();
    }
  }

  if(!m_controlObjects) return;

  for(const auto& co : *m_controlObjects){
    ControlObjectStruct* cos = co.second;
    if(cos->state == CONTROL_IDLE) continue;
    else if (cos->state == CONTROL_SELECTED){
        IedClientError error;
        ControlObjectClient_operateAsync(cos->client,&error, cos->value, 0, controlActionHandler, new std::pair<IEC61850ClientConnection*,ControlObjectStruct*>(this,cos));
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
                            Logger::getLogger()->info("Connecting to %s:%d", m_serverIp.c_str(), m_tcpPort);
                            m_connectionState = CON_STATE_CONNECTING;
                        }

                        else{
                          Logger::getLogger()->error("Failed to connect to %s:%d", m_serverIp.c_str(), m_tcpPort);
                        }
                    }
                    else {
                        m_connectionState = CON_STATE_FATAL_ERROR;
                        Logger::getLogger()->error("Fatal configuration error");

                        m_conLock.unlock();
                    }

                }

                break;

            case CON_STATE_CONNECTING:{
                /* wait for connected event or timeout */
                IedConnectionState newState = IedConnection_getState(m_connection);

                if(newState == IED_STATE_CONNECTED){
                  Logger::getLogger()->info("Connected to %s:%d", m_serverIp.c_str(), m_tcpPort);
                  m_connectionState = CON_STATE_CONNECTED;
                  m_initialiseControlObjects();
                }

                else if (getMonotonicTimeInMs() > m_delayExpirationTime) {
                    Logger::getLogger()->warn("Timeout while connecting");
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

void IEC61850ClientConnection::sendActCon(IEC61850ClientConnection::ControlObjectStruct *cos) {
    m_client->sendCommandAck(cos->label,cos->mode,false);
}

void IEC61850ClientConnection::sendActTerm(IEC61850ClientConnection::ControlObjectStruct *cos) {
    m_client->sendCommandAck(cos->label,cos->mode,true);
}

bool IEC61850ClientConnection::operate(const std::string &objRef, DatapointValue value) {
    auto it = m_controlObjects->find(objRef);

    if(it == m_controlObjects->end()){
        Logger::getLogger()->error("Control object with objRef %s not found", objRef.c_str());
        return false;
    }

    ControlObjectStruct* co = it->second;

    MmsValue* mmsValue = co->value;

    MmsType type = MmsValue_getType(mmsValue);

    switch(type){
        case MMS_BOOLEAN:
            MmsValue_setBoolean(mmsValue, value.toInt());
            break;
        case MMS_INTEGER:
            MmsValue_setInt32(mmsValue,  (int) value.toInt());
            break;
        case MMS_BIT_STRING:
            MmsValue_setBitStringFromInteger(mmsValue, value.toInt());
            break;
        case MMS_FLOAT:
            MmsValue_setFloat(mmsValue,(float) value.toDouble());
            break;
        default:
            Logger::getLogger()->error("Invalid mms value type");
            return false;
    }
    IedClientError error;

    auto connectionControlPair = new std::pair<IEC61850ClientConnection*,ControlObjectStruct*>(this, co);

    if(co->mode == CONTROL_MODEL_DIRECT_ENHANCED || co->mode == CONTROL_MODEL_SBO_ENHANCED){
        ControlObjectClient_setCommandTerminationHandler(co->client, commandTerminationHandler,connectionControlPair);
    }

    switch(co->mode){
        case CONTROL_MODEL_DIRECT_ENHANCED:
        case CONTROL_MODEL_DIRECT_NORMAL:
            co->state = CONTROL_WAIT_FOR_ACT_CON;
            ControlObjectClient_operateAsync(co->client,&error, mmsValue, 0, controlActionHandler, connectionControlPair);
            break;
        case CONTROL_MODEL_SBO_NORMAL:
            co->state = CONTROL_WAIT_FOR_SELECT;
            ControlObjectClient_selectAsync(co->client,&error, controlActionHandler,connectionControlPair);
            break;
        case CONTROL_MODEL_SBO_ENHANCED:
            co->state = CONTROL_WAIT_FOR_SELECT_WITH_VALUE;
            ControlObjectClient_selectWithValueAsync(co->client,&error, mmsValue,controlActionHandler,connectionControlPair);
            break;
        case CONTROL_MODEL_STATUS_ONLY:
            break;
    }

    return true;
}


