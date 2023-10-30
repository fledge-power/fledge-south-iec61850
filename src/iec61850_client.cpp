#include "datapoint.h"
#include "iec61850_client_config.hpp"
#include "iec61850_client_connection.hpp"
#include <iec61850.hpp>
#include <utility>
#include <libiec61850/iec61850_client.h>
#include <libiec61850/hal_thread.h>
#include <libiec61850/iec61850_common.h>
#include "libiec61850/mms_common.h"
#include "libiec61850/mms_value.h"


bool
isCommandCdcType(CDCTYPE type){
    return type>=SPC;
}

static uint64_t
getMonotonicTimeInMs()
{
    uint64_t timeVal = 0;

    struct timespec ts{};

    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        timeVal = ((uint64_t) ts.tv_sec * 1000LL) + (ts.tv_nsec / 1000000);
    }

    return timeVal;
}

static long
getValueInt(Datapoint* dp)
{
    DatapointValue& dpv = dp->getData();

    if (dpv.getType() == DatapointValue::T_INTEGER) {
        return dpv.toInt();
    }
    else {
        Logger::getLogger()->error("Value is not int %s", dp->toJSONProperty().c_str());
    }
  return -1;
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


static Datapoint*
getChild(Datapoint* dp, const std::string& name)
{
    Datapoint* childDp = nullptr;

    DatapointValue& dpv = dp->getData();

    
    if(dpv.getType() != DatapointValue::T_DP_DICT){
      Logger::getLogger()->warn("Datapoint not a dictionary");
      return nullptr;
    }

    std::vector<Datapoint*>* datapoints = dpv.getDpVec();

    if (!datapoints) {
        Logger::getLogger()->warn("datapoints is nullptr");
        return nullptr; 
    }

    for (auto child : *datapoints) {
        if (child->getName() == name) {
            childDp = child;
            break; 
        }
    }

    return childDp;
}

static std::string
getValueStr(Datapoint* dp)
{
    DatapointValue& dpv = dp->getData();

    if (dpv.getType() == DatapointValue::T_STRING) {
        return dpv.toStringValue();
    }
    else {
       Logger::getLogger()->error("datapoint " + dp->getName() + " has mot a std::string value");
    }
    
   return nullptr;
}

static Datapoint*
createDp(const std::string& name)
{
    auto* datapoints = new std::vector<Datapoint*>;

    DatapointValue dpv(datapoints, true);

    auto* dp = new Datapoint(name, dpv);

    return dp;
}

template <class T>
static Datapoint*
createDpWithValue(const std::string& name, const T value)
{
    DatapointValue dpv(value);

    auto* dp = new Datapoint(name, dpv);

    return dp;
}

static Datapoint*
addElement(Datapoint* dp, const std::string& name)
{
    DatapointValue& dpv = dp->getData();

    std::vector<Datapoint*>* subDatapoints = dpv.getDpVec();

    Datapoint* element = createDp(name);

    if (element) {
       subDatapoints->push_back(element);
    }

    return element;
}

template <class T>
static Datapoint*
addElementWithValue(Datapoint* dp, const std::string& name, const T value)
{
    DatapointValue& dpv = dp->getData();

    std::vector<Datapoint*>* subDatapoints = dpv.getDpVec();

    Datapoint* element = createDpWithValue(name, value);

    if (element) {
       subDatapoints->push_back(element);
    }

    return element;
}

std::map<CDCTYPE, std::string> cdcToStrMap = {
    {SPS, "SpsTyp"},
    {DPS, "DpsTyp"},
    {BSC, "BscTyp"},
    {MV, "MvTyp"},
    {SPC, "SpcTyp"},
    {DPC, "DpcTyp"},
    {APC, "ApcTyp"},
    {INC, "IncTyp"}
};

std::map<CDCTYPE, PIVOTROOT> rootMap = {
    {SPS, GTIS}, {DPS, GTIS},
    {BSC, GTIS}, {MV,  GTIM},
    {SPC, GTIC}, {DPS,GTIC},
    {APC,GTIC}, {INC, GTIC}
};

std::map<PIVOTROOT, std::string> rootToStrMap = {
  {GTIM, "GTIM"}, {GTIS, "GTIS"}, {GTIC, "GTIC"}
};


IEC61850Client::IEC61850Client(IEC61850* iec61850, IEC61850ClientConfig* iec61850_client_config)
          :m_iec61850(iec61850),
           m_config(iec61850_client_config)
{}

IEC61850Client::~IEC61850Client()
{
  stop();
  delete m_outstandingCommands;
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

int 
IEC61850Client::getRootFromCDC( const CDCTYPE cdc){
  auto it = rootMap.find(cdc);
  if(it != rootMap.end()) {
    return it->second;
  }
  return -1;
}

void
IEC61850Client::logError(IedClientError err, std::string info)
{
    Logger::getLogger()->error("In here : %s",  info.c_str());
    switch (err) {
        case IED_ERROR_OK:
            Logger::getLogger()->info("No error occurred - service request has been successful");
            break;
        case IED_ERROR_NOT_CONNECTED:
            Logger::getLogger()->error("Service request can't be executed because the client is not yet connected");
            break;
        case IED_ERROR_ALREADY_CONNECTED:
            Logger::getLogger()->error("Connect service not executed because the client is already connected");
            break;
        case IED_ERROR_CONNECTION_LOST:
            Logger::getLogger()->error("Service request can't be executed due to a loss of connection");
            break;
        case IED_ERROR_SERVICE_NOT_SUPPORTED:
            Logger::getLogger()->error("The service or some given parameters are not supported by the client stack or by the server");
            break;
        case IED_ERROR_CONNECTION_REJECTED:
            Logger::getLogger()->error("Connection rejected by server");
            break;
        case IED_ERROR_OUTSTANDING_CALL_LIMIT_REACHED:
            Logger::getLogger()->error("Cannot send request because outstanding call limit is reached");
            break;
        case IED_ERROR_USER_PROVIDED_INVALID_ARGUMENT:
            Logger::getLogger()->error("API function has been called with an invalid argument");
            break;
        case IED_ERROR_ENABLE_REPORT_FAILED_DATASET_MISMATCH:
            Logger::getLogger()->error("Enable report failed due to dataset mismatch");
            break;
        case IED_ERROR_OBJECT_REFERENCE_INVALID:
            Logger::getLogger()->error("Provided object reference is invalid");
            break;
        case IED_ERROR_UNEXPECTED_VALUE_RECEIVED:
            Logger::getLogger()->error("Received object is of unexpected type");
            break;
        case IED_ERROR_TIMEOUT:
            Logger::getLogger()->error("Communication to the server failed with a timeout");
            break;
        case IED_ERROR_ACCESS_DENIED:
            Logger::getLogger()->error("Access to the requested object/service was denied by the server");
            break;
        case IED_ERROR_OBJECT_DOES_NOT_EXIST:
            Logger::getLogger()->error("Server reported that the requested object does not exist");
            break;
        case IED_ERROR_OBJECT_EXISTS:
            Logger::getLogger()->error("Server reported that the requested object already exists");
            break;
        case IED_ERROR_OBJECT_ACCESS_UNSUPPORTED:
            Logger::getLogger()->error("Server does not support the requested access method");
            break;
        case IED_ERROR_TYPE_INCONSISTENT:
            Logger::getLogger()->error("Server expected an object of another type");
            break;
        case IED_ERROR_TEMPORARILY_UNAVAILABLE:
            Logger::getLogger()->error("Object or service is temporarily unavailable");
            break;
        case IED_ERROR_OBJECT_UNDEFINED:
            Logger::getLogger()->error("Specified object is not defined in the server");
            break;
        case IED_ERROR_INVALID_ADDRESS:
            Logger::getLogger()->error("Specified address is invalid");
            break;
        case IED_ERROR_HARDWARE_FAULT:
            Logger::getLogger()->error("Service failed due to a hardware fault");
            break;
        case IED_ERROR_TYPE_UNSUPPORTED:
            Logger::getLogger()->error("Requested data type is not supported by the server");
            break;
        case IED_ERROR_OBJECT_ATTRIBUTE_INCONSISTENT:
            Logger::getLogger()->error("Provided attributes are inconsistent");
            break;
        case IED_ERROR_OBJECT_VALUE_INVALID:
            Logger::getLogger()->error("Provided object value is invalid");
            break;
        case IED_ERROR_OBJECT_INVALIDATED:
            Logger::getLogger()->error("Object is invalidated");
            break;
        case IED_ERROR_MALFORMED_MESSAGE:
            Logger::getLogger()->error("Received an invalid response message from the server");
            break;
        case IED_ERROR_SERVICE_NOT_IMPLEMENTED:
            Logger::getLogger()->error("Service not implemented");
            break;
        case IED_ERROR_UNKNOWN:
            Logger::getLogger()->error("Unknown error");
            break;
    }
}

uint64_t
PivotTimestamp::GetCurrentTimeInMs()
{
    struct timeval now;

    gettimeofday(&now, nullptr);

    return ((uint64_t) now.tv_sec * 1000LL) + (now.tv_usec / 1000);
}

void
PivotTimestamp::handleTimeQuality(Datapoint* timeQuality)
{
    DatapointValue& dpv = timeQuality->getData();

    if (dpv.getType() == DatapointValue::T_DP_DICT)
    {
        std::vector<Datapoint*>* datapoints = dpv.getDpVec();

        for (Datapoint* child : *datapoints)
        {
            if (child->getName() == "clockFailure") {
                if (getValueInt(child) > 0)
                    m_clockFailure = true;
                else
                    m_clockFailure = false;
            }
            else if (child->getName() == "clockNotSynchronized") {
                if (getValueInt(child) > 0)
                    m_clockNotSynchronized = true;
                else
                    m_clockNotSynchronized = false;
            }
            else if (child->getName() == "leapSecondKnown") {
                if (getValueInt(child) > 0)
                    m_leapSecondKnown = true;
                else
                    m_leapSecondKnown = false;
            }
            else if (child->getName() == "timeAccuracy") {
                m_timeAccuracy = getValueInt(child);
            }
        }
    }
}

PivotTimestamp::PivotTimestamp(Datapoint* timestampData)
{
    DatapointValue& dpv = timestampData->getData();
    m_valueArray = new uint8_t[7];

    if (dpv.getType() == DatapointValue::T_DP_DICT)
    {
        std::vector<Datapoint*>* datapoints = dpv.getDpVec();

        for (Datapoint* child : *datapoints)
        {
            if (child->getName() == "SecondSinceEpoch") {
                uint32_t secondSinceEpoch = getValueInt(child);

                m_valueArray[0] = (secondSinceEpoch / 0x1000000 & 0xff);
                m_valueArray[1] = (secondSinceEpoch / 0x10000 & 0xff);
                m_valueArray[2] = (secondSinceEpoch / 0x100 & 0xff);
                m_valueArray[3] = (secondSinceEpoch & 0xff);
            }
            else if (child->getName() == "FractionOfSecond") {
                uint32_t fractionOfSecond = getValueInt(child);

                m_valueArray[4] = ((fractionOfSecond >> 16) & 0xff);
                m_valueArray[5] = ((fractionOfSecond >> 8) & 0xff);
                m_valueArray[6] = (fractionOfSecond & 0xff);
            }
            else if (child->getName() == "TimeQuality") {
                handleTimeQuality(child);
            }
        }
    }
}

PivotTimestamp::PivotTimestamp(uint64_t ms)
{
    m_valueArray = new uint8_t[7];
    uint32_t timeval32 = (uint32_t) (ms/ 1000LL);

    m_valueArray[0] = (timeval32 / 0x1000000 & 0xff);
    m_valueArray[1] = (timeval32 / 0x10000 & 0xff);
    m_valueArray[2] = (timeval32 / 0x100 & 0xff);
    m_valueArray[3] = (timeval32 & 0xff);

    uint32_t remainder = (ms % 1000LL);
    uint32_t fractionOfSecond = (remainder) * 16777 + ((remainder * 216) / 1000);

    m_valueArray[4] = ((fractionOfSecond >> 16) & 0xff);
    m_valueArray[5] = ((fractionOfSecond >> 8) & 0xff);
    m_valueArray[6] = (fractionOfSecond & 0xff);
}

PivotTimestamp::~PivotTimestamp()
{
    delete[] m_valueArray;
}

void
PivotTimestamp::setTimeInMs(uint64_t ms){
    auto timeval32 = (uint32_t) (ms/ 1000LL);

    m_valueArray[0] = (timeval32 / 0x1000000 & 0xff);
    m_valueArray[1] = (timeval32 / 0x10000 & 0xff);
    m_valueArray[2] = (timeval32 / 0x100 & 0xff);
    m_valueArray[3] = (timeval32 & 0xff);

    uint32_t remainder = (ms % 1000LL);
    uint32_t fractionOfSecond = (remainder) * 16777 + ((remainder * 216) / 1000);

    m_valueArray[4] = ((fractionOfSecond >> 16) & 0xff);
    m_valueArray[5] = ((fractionOfSecond >> 8) & 0xff);
    m_valueArray[6] = (fractionOfSecond & 0xff);
}

uint64_t
PivotTimestamp::getTimeInMs(){
    uint32_t timeval32;

    timeval32 = m_valueArray[3];
    timeval32 += m_valueArray[2] * 0x100;
    timeval32 += m_valueArray[1] * 0x10000;
    timeval32 += m_valueArray[0] * 0x1000000;

    uint32_t fractionOfSecond = 0;

    fractionOfSecond = (m_valueArray[4] << 16);
    fractionOfSecond += (m_valueArray[5] << 8);
    fractionOfSecond += (m_valueArray[6]);

    uint32_t remainder = fractionOfSecond / 16777;

    uint64_t msVal = (timeval32 * 1000LL) + remainder;

    return (uint64_t) msVal;
}

int
PivotTimestamp::FractionOfSecond(){
    uint32_t fractionOfSecond = 0;

    fractionOfSecond = (m_valueArray[4] << 16);
    fractionOfSecond += (m_valueArray[5] << 8);
    fractionOfSecond += (m_valueArray[6]);

    return fractionOfSecond;
}

int
PivotTimestamp::SecondSinceEpoch(){
    int32_t timeval32;

    timeval32 = m_valueArray[3];
    timeval32 += m_valueArray[2] * 0x100;
    timeval32 += m_valueArray[1] * 0x10000;
    timeval32 += m_valueArray[0] * 0x1000000;

    return timeval32;
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
            m_active_connection = clientConnection;
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

            if (m_active_connection->Connected() == false){

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

void
IEC61850Client::sendData(std::vector<Datapoint*> datapoints,
                            const std::vector<std::string> labels)
{
    int i = 0;

    for (Datapoint* item_dp : datapoints)
    {
        std::vector<Datapoint*> points;
        points.push_back(item_dp);

        m_iec61850->ingest(labels.at(i), points);
        i++;
    }
}

void
IEC61850Client::handleValues()
{
  std::vector<std::string> labels;
  std::vector<Datapoint*> datapoints;

  for(auto pair: *m_config->ExchangeDefinition()){
    DataExchangeDefinition* def = pair.second;

    CDCTYPE typeId = def->cdcType;
    
    if(isCommandCdcType(typeId)) continue;

    labels.push_back(def->label);

    m_handleMonitoringData(def->objRef, datapoints, def->label, typeId);
  }        
  sendData(datapoints, labels);
}

void
IEC61850Client::m_handleMonitoringData(std::string objRef, std::vector<Datapoint*>& datapoints, std::string& label, CDCTYPE type)
{
  IedClientError error;
  if(!m_active_connection) {
    Logger::getLogger()->error("No active connection");
    return;
  }
  
  MmsValue* mmsvalue = m_active_connection->readValue(&error, objRef.c_str());

  if(error!=IED_ERROR_OK){
    logError(error, "Get MmsValue " + objRef ); 
    return;
  }
  MmsVariableSpecification* varSpec = m_active_connection->getVariableSpec(&error, objRef.c_str());

  if(error!=IED_ERROR_OK){
    logError(error, "Get MmsVarSpec " + objRef ); 
    return;
  }

  MmsValue* qualityMms = MmsValue_getSubElement(mmsvalue, varSpec,(char*) "q");
  if(!qualityMms){
    Logger::getLogger()->error("No quality found");
    return;
  }
  Quality quality = Quality_fromMmsValue(qualityMms);

  MmsValue* timestampMms = MmsValue_getSubElement(mmsvalue, varSpec, (char*) "t");
  if(!timestampMms){
    Logger::getLogger()->error("No timestamp found");
  }
  uint64_t timestamp = MmsValue_getUtcTimeInMs(timestampMms);
  
  switch(type){
    case SPS:{
      MmsValue* stVal = MmsValue_getSubElement(mmsvalue, varSpec, (char*) "stVal");
      if(!stVal){
        Logger::getLogger()->error("No value found %s", objRef.c_str());
        return;
      } 
      bool value = MmsValue_getBoolean(stVal);
      datapoints.push_back(m_createDatapoint(label,objRef,(long)value, quality, timestamp));
      return;
    }
    case DPS:{
      return;
    }
    case BSC:{
      return;
    }
    case MV:{
      MmsValue* mag = MmsValue_getSubElement(mmsvalue, varSpec, (char *)"mag");
      if(!mag){
        Logger::getLogger()->error("No mag found %s", objRef.c_str());
        return;
      }
      MmsValue* i = MmsValue_getSubElement(mag, varSpec, (char *) "i");
      if(i){
        long value = MmsValue_toInt32(i); 
        datapoints.push_back(m_createDatapoint(label,objRef,(long)value, quality, timestamp));
        return;
      }
      MmsValue* f = MmsValue_getSubElement(mag, varSpec, (char *) "f");
      if(f){
        double value = MmsValue_toFloat(f) ; 
        datapoints.push_back(m_createDatapoint(label,objRef,(double)value, quality, timestamp));
        return;
      }
      Logger::getLogger()->error("No value found %s", objRef.c_str());
      return;
    }
    case ENS:
    case INS: {
      MmsValue* stVal = MmsValue_getSubElement(mmsvalue, varSpec, (char*)"stVal");
      if(!stVal){
        Logger::getLogger()->error("No stVal found %s", objRef.c_str());
        return;
      }
      long value = MmsValue_toInt32(stVal);
      datapoints.push_back(m_createDatapoint(label,objRef,(long)value, quality, timestamp));
      return;
    }
    default:{
      Logger::getLogger()->error("Invalid cdc type %s" ,objRef.c_str());
    }
  }
  
}


template <class T>
Datapoint*
IEC61850Client::m_createDatapoint(std::string& label, std::string objRef, T value, Quality quality, uint64_t timestamp){
  DataExchangeDefinition* def = m_config->getExchangeDefinitionByLabel(label);  
   
  Datapoint* pivotDp = createDp("PIVOT");

  PIVOTROOT root = (PIVOTROOT) getRootFromCDC(def->cdcType);

  Datapoint* rootDp = addElement(pivotDp,rootToStrMap[root]);
  Datapoint* comingFromDp = addElementWithValue(rootDp, "ComingFrom", (std::string) "iec61850");
  Datapoint* identifierDp = addElementWithValue(rootDp, "Identifier", (std::string) def->label);
  Datapoint* cdcDp = addElement(rootDp, cdcToStrMap[def->cdcType]);
  
  addValueDp(cdcDp, def->cdcType, value);
  addQualityDp(cdcDp, quality);   
  addTimestampDp(cdcDp, timestamp);

  return pivotDp;
}

void 
IEC61850Client::addQualityDp(Datapoint* cdcDp, Quality quality)
{
  Datapoint* qualityDp = addElement(cdcDp, "q");
  addElementWithValue(qualityDp, "test", (long) Quality_isFlagSet(&quality, QUALITY_TEST));
  
  Validity val = Quality_getValidity(&quality);
  if(val == QUALITY_VALIDITY_GOOD){
    addElementWithValue(qualityDp, "Validity", (std::string) "good");
  }
  else if(val ==  QUALITY_VALIDITY_INVALID){
    addElementWithValue(qualityDp, "Validity", (std::string) "invalid");
  }
  else if(val ==  QUALITY_VALIDITY_RESERVED){
    addElementWithValue(qualityDp, "Validity", (std::string) "reserved");
  }
  else if( val == QUALITY_VALIDITY_QUESTIONABLE){
    addElementWithValue(qualityDp, "Validity", (std::string) "questionable");
  }

  Datapoint* detailQualityDp = addElement(qualityDp, "DetailQuality");

  if(Quality_isFlagSet(&quality, QUALITY_DETAIL_OVERFLOW)){
    addElementWithValue(detailQualityDp,"overflow", (long) true);
  }
  if(Quality_isFlagSet(&quality, QUALITY_DETAIL_OLD_DATA)){
      addElementWithValue(detailQualityDp,"oldData", (long) true);
  }
  if(Quality_isFlagSet(&quality, QUALITY_DETAIL_OUT_OF_RANGE)){
      addElementWithValue(detailQualityDp,"outOfRange", (long) true);
  }
  
  if(Quality_isFlagSet(&quality,QUALITY_OPERATOR_BLOCKED)){
      addElementWithValue(qualityDp,"operatorBlocked", (long) true);
  }

  if(Quality_isFlagSet(&quality, QUALITY_SOURCE_SUBSTITUTED)){
    addElementWithValue(qualityDp, "Source", (std::string) "substituted");
  }
}
    
void
IEC61850Client::addTimestampDp(Datapoint* cdcDp, uint64_t timestampMs)
{ 
  PivotTimestamp* ts = new PivotTimestamp(timestampMs);
  Datapoint* tsDp = addElement(cdcDp, "t");
  addElementWithValue(tsDp, "SecondSinceEpoch" , (long) ts->SecondSinceEpoch());
  addElementWithValue(tsDp, "FractionOfSecond" , (long) ts->FractionOfSecond());

  delete ts;
}
    
template <class T> void 
IEC61850Client::addValueDp(Datapoint* cdcDp, CDCTYPE type, T value)
{
  switch(type){
    case ENS:
    case INS:
    case SPS:{
      addElementWithValue(cdcDp, "stVal", (long) value);
      break;
    }
    case DPS:{
      long valueInt = (long) value;
      std::string stVal;
      if     (valueInt == 0) stVal = "intermediate-state";
      else if(valueInt == 1) stVal = "off";
      else if(valueInt == 2) stVal = "on";
      else if(valueInt == 3) stVal = "bad-state";
      addElementWithValue(cdcDp, "stVal", (std::string) stVal);
      break;
    }
    case MV:{
      Datapoint* magDp = addElement(cdcDp, "mag");
      if(std::is_same<T,double>::value){
         addElementWithValue(magDp,"f",(double) value); 
      }
      else if(std::is_same<T, long>::value){
         addElementWithValue(magDp,"i",(long) value); 
      }
      else{
        Logger::getLogger()->error("Invalid mag data type");
      }
      break;
    }
    case BSC:{
        break;
    }
    default:
      {
        Logger::getLogger()->error("Invalid cdcType %d", type);
        break;
      }
  }
}

bool
IEC61850Client::handleOperation(Datapoint* operation){
  if(!m_outstandingCommands) m_outstandingCommands = new std::unordered_map<std::string, Datapoint*>();

  Datapoint* identifierDp = getChild(operation, "Identifier");

  if(!identifierDp || identifierDp->getData().getType() != DatapointValue::T_STRING){
    Logger::getLogger()->warn("Operation has no identifier");
    return false;
  }

  std::string id = getValueStr(identifierDp);

  DataExchangeDefinition* def = m_config->getExchangeDefinitionByPivotId(id);

  if(!def){
    Logger::getLogger()->warn("No exchange definition found for pivot id %s", id.c_str());
    return false;
  }

  std::string label = def->label;

  std::string objRef = def->objRef;

  Datapoint* cdcDp = getCdc(operation);

  if(!cdcDp){
      Logger::getLogger()->error("Operation has no cdc");
      return false;
  }

  Datapoint* valueDp = getChild(cdcDp, "ctlVal");

  if(!valueDp){
      Logger::getLogger()->error("Operation has no value");
      return false;
  }

  DatapointValue value = valueDp->getData();

  bool res =  m_active_connection->operate(objRef,value);

  (*m_outstandingCommands)[label] = operation;

  return res;
}

void IEC61850Client::sendCommandAck(const std::string& label, ControlModel mode, bool terminated) {
    if(!m_outstandingCommands){
        Logger::getLogger()->error("No outstanding commands");
        return;
    }

    auto it = m_outstandingCommands->find(label);
    if(it == m_outstandingCommands->end()){
        Logger::getLogger()->error("No outstanding command with label %s found", label.c_str());
        return;
    }

    Datapoint* pivotRoot = createDp("PIVOT");
    Datapoint* command = addElementWithValue(pivotRoot,"GTIC", it->second->getData());
    int cot = terminated ? 10 : 7;
    Datapoint* causeDp = nullptr;
    causeDp = getChild(command,"Cause");
    if(causeDp){
        Datapoint* stVal = getChild(causeDp,"stVal");
        if(!stVal){
            Logger::getLogger()->error("Cause dp has no stVal");
            return;
        }
        stVal->getData().setValue((long) cot);
    }
    else {
        causeDp = addElement(command, "Cause");
        addElementWithValue(causeDp, "stVal", (long) cot);
    }

    std::vector<Datapoint*> datapoints;
    std::vector<std::string> labels;
    labels.push_back(label);
    datapoints.push_back(pivotRoot);
    sendData(datapoints,labels);

    if (terminated || (!terminated && (mode == CONTROL_MODEL_SBO_NORMAL || mode == CONTROL_MODEL_DIRECT_NORMAL))) {
        m_outstandingCommands->erase(label);
    }
}

