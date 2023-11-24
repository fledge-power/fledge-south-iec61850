#include "datapoint.h"
#include "iec61850_client_config.hpp"
#include "iec61850_client_connection.hpp"
#include "libiec61850/mms_common.h"
#include "libiec61850/mms_value.h"
#include <iec61850.hpp>
#include <libiec61850/hal_thread.h>
#include <libiec61850/iec61850_client.h>
#include <libiec61850/iec61850_common.h>
#include <libiec61850/mms_type_spec.h>
#include <utility>

FunctionalConstraint
stringToFunctionalConstraint (const std::string& str)
{
    static const std::unordered_map<std::string, FunctionalConstraint> mapping{
        { "ST", IEC61850_FC_ST },    { "MX", IEC61850_FC_MX },
        { "SP", IEC61850_FC_SP },    { "SV", IEC61850_FC_SV },
        { "CF", IEC61850_FC_CF },    { "DC", IEC61850_FC_DC },
        { "SG", IEC61850_FC_SG },    { "SE", IEC61850_FC_SE },
        { "SR", IEC61850_FC_SR },    { "OR", IEC61850_FC_OR },
        { "BL", IEC61850_FC_BL },    { "EX", IEC61850_FC_EX },
        { "CO", IEC61850_FC_CO },    { "US", IEC61850_FC_US },
        { "MS", IEC61850_FC_MS },    { "RP", IEC61850_FC_RP },
        { "BR", IEC61850_FC_BR },    { "LG", IEC61850_FC_LG },
        { "GO", IEC61850_FC_GO },    { "ALL", IEC61850_FC_ALL },
        { "NONE", IEC61850_FC_NONE }
    };
    auto it = mapping.find (str);
    if (it != mapping.end ())
    {
        return it->second;
    }
    else
    {
        return IEC61850_FC_NONE;
    }
}

bool
isCommandCdcType (CDCTYPE type)
{
    return type >= SPC;
}

static uint64_t
getMonotonicTimeInMs ()
{
    uint64_t timeVal = 0;

    struct timespec ts;

    if (clock_gettime (CLOCK_MONOTONIC, &ts) == 0)
    {
        timeVal = ((uint64_t)ts.tv_sec * 1000LL) + (ts.tv_nsec / 1000000);
    }

    return timeVal;
}

static long
getValueInt (Datapoint* dp)
{
    DatapointValue const& dpv = dp->getData ();

    if (dpv.getType () == DatapointValue::T_INTEGER)
    {
        return dpv.toInt ();
    }
    else
    {
        Iec61850Utility::log_error ("Value is not int %s",
                                    dp->toJSONProperty ().c_str ());
    }
    return -1;
}

static Datapoint*
getCdc (Datapoint* dp)
{
    DatapointValue& dpv = dp->getData ();

    if (dpv.getType () != DatapointValue::T_DP_DICT)
    {
        Iec61850Utility::log_error ("Datapoint is not a dictionary %s",
                                    dp->getName ().c_str ());
    }

    std::vector<Datapoint*> const* datapoints = dpv.getDpVec ();

    for (Datapoint* child : *datapoints)
    {
        if (IEC61850ClientConfig::getCdcTypeFromString (child->getName ())
            != -1)
        {
            return child;
        }
    }

    return nullptr;
}

static Datapoint*
getChild (Datapoint* dp, const std::string& name)
{
    Datapoint* childDp = nullptr;

    DatapointValue& dpv = dp->getData ();

    if (dpv.getType () != DatapointValue::T_DP_DICT)
    {
        Iec61850Utility::log_warn ("Datapoint not a dictionary");
        return nullptr;
    }

    std::vector<Datapoint*> const* datapoints = dpv.getDpVec ();

    if (!datapoints)
    {
        Iec61850Utility::log_warn ("datapoints is nullptr");
        return nullptr;
    }

    for (auto child : *datapoints)
    {
        if (child->getName () == name)
        {
            childDp = child;
            break;
        }
    }

    return childDp;
}

static std::string
getValueStr (Datapoint* dp)
{
    const DatapointValue& dpv = dp->getData ();

    if (dpv.getType () == DatapointValue::T_STRING)
    {
        return dpv.toStringValue ();
    }
    else
    {
        Iec61850Utility::log_error (
            ("datapoint " + dp->getName () + " has mot a std::string value")
                .c_str ());
    }

    return nullptr;
}

static Datapoint*
createDp (const std::string& name)
{
    auto* datapoints = new std::vector<Datapoint*>;

    DatapointValue dpv (datapoints, true);

    auto* dp = new Datapoint (name, dpv);

    return dp;
}

template <class T>
static Datapoint*
createDpWithValue (const std::string& name, const T value)
{
    DatapointValue dpv (value);

    auto* dp = new Datapoint (name, dpv);

    return dp;
}

static Datapoint*
addElement (Datapoint* dp, const std::string& name)
{
    DatapointValue& dpv = dp->getData ();

    std::vector<Datapoint*>* subDatapoints = dpv.getDpVec ();

    Datapoint* element = createDp (name);

    if (element)
    {
        subDatapoints->push_back (element);
    }

    return element;
}

template <class T>
static Datapoint*
addElementWithValue (Datapoint* dp, const std::string& name, const T value)
{
    DatapointValue& dpv = dp->getData ();

    std::vector<Datapoint*>* subDatapoints = dpv.getDpVec ();

    Datapoint* element = createDpWithValue (name, value);

    if (element)
    {
        subDatapoints->push_back (element);
    }

    return element;
}

const std::map<CDCTYPE, std::string> cdcToStrMap
    = { { SPS, "SpsTyp" }, { DPS, "DpsTyp" }, { BSC, "BscTyp" },
        { MV, "MvTyp" },   { SPC, "SpcTyp" }, { DPC, "DpcTyp" },
        { APC, "ApcTyp" }, { INC, "IncTyp" }, { INS, "InsTyp" },
        { ENS, "EnsTyp" } };
const std::map<CDCTYPE, PIVOTROOT> rootMap
    = { { SPS, GTIS }, { DPS, GTIS }, { BSC, GTIC }, { INS, GTIS },
        { ENS, GTIS }, { MV, GTIM },  { SPC, GTIC }, { DPC, GTIC },
        { APC, GTIC }, { INC, GTIC } };

const std::map<PIVOTROOT, std::string> rootToStrMap
    = { { GTIM, "GTIM" }, { GTIS, "GTIS" }, { GTIC, "GTIC" } };

IEC61850Client::IEC61850Client (IEC61850* iec61850,
                                IEC61850ClientConfig* iec61850_client_config)
    : m_config (iec61850_client_config), m_iec61850 (iec61850)
{
}

IEC61850Client::~IEC61850Client () { stop (); }

void
IEC61850Client::stop ()
{
    if (!m_started)
    {
        return;
    }

    m_started = false;

    if (m_monitoringThread != nullptr)
    {
        m_monitoringThread->join ();
        delete m_monitoringThread;
        m_monitoringThread = nullptr;
    }
}

int
IEC61850Client::getRootFromCDC (const CDCTYPE cdc)
{
    auto it = rootMap.find (cdc);
    if (it != rootMap.end ())
    {
        return it->second;
    }
    return -1;
}

// LCOV_EXCL_START
void
IEC61850Client::logIedClientError (IedClientError err,
                                   const std::string& info) const
{
    Iec61850Utility::log_error ("In here : %s", info.c_str ());
    switch (err)
    {
    case IED_ERROR_OK:
        Iec61850Utility::log_info (
            "No error occurred - service request has been successful");
        break;
    case IED_ERROR_NOT_CONNECTED:
        Iec61850Utility::log_error (
            "Service request can't be executed because the client is not "
            "yet connected");
        break;
    case IED_ERROR_ALREADY_CONNECTED:
        Iec61850Utility::log_error ("Connect service not executed because "
                                    "the client is already connected");
        break;
    case IED_ERROR_CONNECTION_LOST:
        Iec61850Utility::log_error ("Service request can't be executed "
                                    "due to a loss of connection");
        break;
    case IED_ERROR_SERVICE_NOT_SUPPORTED:
        Iec61850Utility::log_error (
            "The service or some given parameters are not supported by "
            "the client stack or by the server");
        break;
    case IED_ERROR_CONNECTION_REJECTED:
        Iec61850Utility::log_error ("Connection rejected by server");
        break;
    case IED_ERROR_OUTSTANDING_CALL_LIMIT_REACHED:
        Iec61850Utility::log_error ("Cannot send request because "
                                    "outstanding call limit is reached");
        break;
    case IED_ERROR_USER_PROVIDED_INVALID_ARGUMENT:
        Iec61850Utility::log_error (
            "API function has been called with an invalid argument");
        break;
    case IED_ERROR_ENABLE_REPORT_FAILED_DATASET_MISMATCH:
        Iec61850Utility::log_error (
            "Enable report failed due to dataset mismatch");
        break;
    case IED_ERROR_OBJECT_REFERENCE_INVALID:
        Iec61850Utility::log_error ("Provided object reference is invalid");
        break;
    case IED_ERROR_UNEXPECTED_VALUE_RECEIVED:
        Iec61850Utility::log_error ("Received object is of unexpected type");
        break;
    case IED_ERROR_TIMEOUT:
        Iec61850Utility::log_error (
            "Communication to the server failed with a timeout");
        break;
    case IED_ERROR_ACCESS_DENIED:
        Iec61850Utility::log_error (
            "Access to the requested object/service was denied by the "
            "server");
        break;
    case IED_ERROR_OBJECT_DOES_NOT_EXIST:
        Iec61850Utility::log_error (
            "Server reported that the requested object does not exist");
        break;
    case IED_ERROR_OBJECT_EXISTS:
        Iec61850Utility::log_error (
            "Server reported that the requested object already exists");
        break;
    case IED_ERROR_OBJECT_ACCESS_UNSUPPORTED:
        Iec61850Utility::log_error (
            "Server does not support the requested access method");
        break;
    case IED_ERROR_TYPE_INCONSISTENT:
        Iec61850Utility::log_error (
            "Server expected an object of another type");
        break;
    case IED_ERROR_TEMPORARILY_UNAVAILABLE:
        Iec61850Utility::log_error (
            "Object or service is temporarily unavailable");
        break;
    case IED_ERROR_OBJECT_UNDEFINED:
        Iec61850Utility::log_error (
            "Specified object is not defined in the server");
        break;
    case IED_ERROR_INVALID_ADDRESS:
        Iec61850Utility::log_error ("Specified address is invalid");
        break;
    case IED_ERROR_HARDWARE_FAULT:
        Iec61850Utility::log_error ("Service failed due to a hardware fault");
        break;
    case IED_ERROR_TYPE_UNSUPPORTED:
        Iec61850Utility::log_error (
            "Requested data type is not supported by the server");
        break;
    case IED_ERROR_OBJECT_ATTRIBUTE_INCONSISTENT:
        Iec61850Utility::log_error ("Provided attributes are inconsistent");
        break;
    case IED_ERROR_OBJECT_VALUE_INVALID:
        Iec61850Utility::log_error ("Provided object value is invalid");
        break;
    case IED_ERROR_OBJECT_INVALIDATED:
        Iec61850Utility::log_error ("Object is invalidated");
        break;
    case IED_ERROR_MALFORMED_MESSAGE:
        Iec61850Utility::log_error (
            "Received an invalid response message from the server");
        break;
    case IED_ERROR_SERVICE_NOT_IMPLEMENTED:
        Iec61850Utility::log_error ("Service not implemented");
        break;
    case IED_ERROR_UNKNOWN:
        Iec61850Utility::log_error ("Unknown error");
        break;
    }
}

uint64_t
PivotTimestamp::GetCurrentTimeInMs ()
{
    struct timeval now;

    gettimeofday (&now, nullptr);

    return ((uint64_t)now.tv_sec * 1000LL) + (now.tv_usec / 1000);
}

void
PivotTimestamp::handleTimeQuality (Datapoint* timeQuality)
{
    DatapointValue& dpv = timeQuality->getData ();

    if (dpv.getType () == DatapointValue::T_DP_DICT)
    {
        std::vector<Datapoint*> const* datapoints = dpv.getDpVec ();

        for (Datapoint* child : *datapoints)
        {
            if (child->getName () == "clockFailure")
            {
                m_clockFailure = getValueInt (child) > 0;
            }
            else if (child->getName () == "clockNotSynchronized")
            {
                m_clockNotSynchronized = getValueInt (child) > 0;
            }
            else if (child->getName () == "leapSecondKnown")
            {
                m_leapSecondKnown = getValueInt (child) > 0;
            }
            else if (child->getName () == "timeAccuracy")
            {
                m_timeAccuracy = (int)getValueInt (child);
            }
        }
    }
}

PivotTimestamp::PivotTimestamp (Datapoint* timestampData)
{
    DatapointValue& dpv = timestampData->getData ();

    if (dpv.getType () == DatapointValue::T_DP_DICT)
    {
        std::vector<Datapoint*> const* datapoints = dpv.getDpVec ();

        for (Datapoint* child : *datapoints)
        {
            if (child->getName () == "SecondSinceEpoch")
            {
                auto secondSinceEpoch = (uint32_t)getValueInt (child);

                m_valueArray[0] = (secondSinceEpoch / 0x1000000 & 0xff);
                m_valueArray[1] = (secondSinceEpoch / 0x10000 & 0xff);
                m_valueArray[2] = (secondSinceEpoch / 0x100 & 0xff);
                m_valueArray[3] = (secondSinceEpoch & 0xff);
            }
            else if (child->getName () == "FractionOfSecond")
            {
                auto fractionOfSecond = (uint32_t)getValueInt (child);

                m_valueArray[4] = ((fractionOfSecond >> 16) & 0xff);
                m_valueArray[5] = ((fractionOfSecond >> 8) & 0xff);
                m_valueArray[6] = (fractionOfSecond & 0xff);
            }
            else if (child->getName () == "TimeQuality")
            {
                handleTimeQuality (child);
            }
        }
    }
}

PivotTimestamp::PivotTimestamp (uint64_t ms)
{
    auto timeval32 = (uint32_t) (ms / 1000LL);

    m_valueArray[0] = (timeval32 / 0x1000000 & 0xff);
    m_valueArray[1] = (timeval32 / 0x10000 & 0xff);
    m_valueArray[2] = (timeval32 / 0x100 & 0xff);
    m_valueArray[3] = (timeval32 & 0xff);

    uint32_t remainder = (ms % 1000LL);
    uint32_t fractionOfSecond = remainder * 16777 + ((remainder * 216) / 1000);

    m_valueArray[4] = ((fractionOfSecond >> 16) & 0xff);
    m_valueArray[5] = ((fractionOfSecond >> 8) & 0xff);
    m_valueArray[6] = (fractionOfSecond & 0xff);
}

void
PivotTimestamp::setTimeInMs (uint64_t ms)
{
    auto timeval32 = (uint32_t) (ms / 1000LL);

    m_valueArray[0] = (timeval32 / 0x1000000 & 0xff);
    m_valueArray[1] = (timeval32 / 0x10000 & 0xff);
    m_valueArray[2] = (timeval32 / 0x100 & 0xff);
    m_valueArray[3] = (timeval32 & 0xff);

    uint32_t remainder = (ms % 1000LL);
    uint32_t fractionOfSecond = remainder * 16777 + ((remainder * 216) / 1000);

    m_valueArray[4] = ((fractionOfSecond >> 16) & 0xff);
    m_valueArray[5] = ((fractionOfSecond >> 8) & 0xff);
    m_valueArray[6] = (fractionOfSecond & 0xff);
}

uint64_t
PivotTimestamp::getTimeInMs () const
{
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

    return msVal;
}

int
PivotTimestamp::FractionOfSecond () const
{
    uint32_t fractionOfSecond = 0;

    fractionOfSecond = (m_valueArray[4] << 16);
    fractionOfSecond += (m_valueArray[5] << 8);
    fractionOfSecond += (m_valueArray[6]);

    return fractionOfSecond;
}

int
PivotTimestamp::SecondSinceEpoch () const
{
    int32_t timeval32;

    timeval32 = m_valueArray[3];
    timeval32 += m_valueArray[2] * 0x100;
    timeval32 += m_valueArray[1] * 0x10000;
    timeval32 += m_valueArray[0] * 0x1000000;

    return timeval32;
}
// LCOV_EXCL_STOP

void
IEC61850Client::start ()
{
    if (m_started)
        return;

    prepareConnections ();
    m_started = true;
    m_monitoringThread
        = new std::thread (&IEC61850Client::_monitoringThread, this);
}

void
IEC61850Client::prepareConnections ()
{
    std::lock_guard<std::mutex> lock (connectionsMutex);
    m_connections
        = std::make_shared<std::vector<IEC61850ClientConnection*> > ();
    for (const auto redgroup : m_config->GetConnections ())
    {
        Iec61850Utility::log_info ("Add connection: %s",
                                   redgroup->ipAddr.c_str ());
        OsiParameters* osiParameters = nullptr;
        if (redgroup->isOsiParametersEnabled)
            osiParameters = &redgroup->osiParameters;
        auto connection = new IEC61850ClientConnection (
            this, m_config, redgroup->ipAddr, redgroup->tcpPort, redgroup->tls,
            osiParameters);

        m_connections->push_back (connection);
    }
}

void
IEC61850Client::updateConnectionStatus (ConnectionStatus newState)
{
    std::lock_guard<std::mutex> lock (statusMutex);
    if (m_connStatus == newState)
        return;

    m_connStatus = newState;
}

void
IEC61850Client::_monitoringThread ()
{
    uint64_t qualityUpdateTimeout = 500; /* 500 ms */
    uint64_t qualityUpdateTimer = 0;
    bool qualityUpdated = false;
    bool firstConnected = false;

    if (m_started)
    {
        std::lock_guard<std::mutex> lock (m_activeConnectionMtx);
        for (auto clientConnection : *m_connections)
        {
            clientConnection->Start ();
            m_active_connection = clientConnection;
        }
    }

    updateConnectionStatus (ConnectionStatus::NOT_CONNECTED);

    uint64_t backupConnectionStartTime
        = Hal_getTimeInMs () + BACKUP_CONNECTION_TIMEOUT;

    while (m_started)
    {
        std::lock_guard<std::mutex> lock (m_activeConnectionMtx);

        if (m_active_connection == nullptr)
        {
            bool foundOpenConnections = false;

            for (auto clientConnection : *m_connections)
            {
                backupConnectionStartTime
                    = Hal_getTimeInMs () + BACKUP_CONNECTION_TIMEOUT;

                foundOpenConnections = true;

                clientConnection->Connect ();

                m_active_connection = clientConnection;

                updateConnectionStatus (ConnectionStatus::STARTED);

                break;
            }

            if (foundOpenConnections)
            {
                firstConnected = true;
                qualityUpdateTimer = 0;
                qualityUpdated = false;
            }

            if (foundOpenConnections == false)
            {

                if (firstConnected)
                {

                    if (qualityUpdated == false)
                    {
                        if (qualityUpdateTimer != 0)
                        {
                            if (getMonotonicTimeInMs () > qualityUpdateTimer)
                            {
                                qualityUpdated = true;
                            }
                        }
                        else
                        {
                            qualityUpdateTimer = getMonotonicTimeInMs ()
                                                 + qualityUpdateTimeout;
                        }
                    }
                }

                updateConnectionStatus (ConnectionStatus::NOT_CONNECTED);

                if (Hal_getTimeInMs () > backupConnectionStartTime)
                {
                    std::lock_guard<std::mutex> lock (m_activeConnectionMtx);
                    for (auto& clientConnection : *m_connections)
                    {
                        if (clientConnection->Disconnected ())
                        {
                            clientConnection->Connect ();
                        }
                    }

                    backupConnectionStartTime
                        = Hal_getTimeInMs () + BACKUP_CONNECTION_TIMEOUT;
                }
            }
        }
        else
        {
            backupConnectionStartTime
                = Hal_getTimeInMs () + BACKUP_CONNECTION_TIMEOUT;
        }

        Thread_sleep (100);
    }

    for (auto& clientConnection : *m_connections)
    {
        std::lock_guard<std::mutex> lock (m_activeConnectionMtx);
        delete clientConnection;
    }

    m_connections->clear ();
}

void
IEC61850Client::sendData (const std::vector<Datapoint*>& datapoints,
                          const std::vector<std::string>& labels)
{
    int i = 0;

    for (Datapoint* item_dp : datapoints)
    {
        std::vector<Datapoint*> points;
        points.push_back (item_dp);

        m_iec61850->ingest (labels.at (i), points);
        i++;
    }
}

void
IEC61850Client::handleAllValues ()
{
    std::vector<std::string> labels;
    std::vector<Datapoint*> datapoints;

    for (const auto& pair : m_config->polledDatapoints ())
    {
        const std::shared_ptr<DataExchangeDefinition> def = pair.second;

        CDCTYPE typeId = def->cdcType;

        labels.push_back (def->label);
        FunctionalConstraint fc = def->cdcType == MV || def->cdcType == APC
                                      ? IEC61850_FC_MX
                                      : IEC61850_FC_ST;

        m_handleMonitoringData (def->objRef, datapoints, def->label, typeId,
                                nullptr, "", fc, 0);
    }
    sendData (datapoints, labels);
}

void
IEC61850Client::handleValue (std::string objRef, MmsValue* mmsValue,
                             uint64_t timestamp)
{
    std::vector<std::string> labels;
    std::vector<Datapoint*> datapoints;

    Iec61850Utility::log_debug ("Handle value %s", objRef.c_str ());

    size_t secondDotPos = objRef.find ('.', objRef.find ('.') + 1);
    size_t bracketPos = objRef.find ('[');

    if (bracketPos == std::string::npos)
    {
        Iec61850Utility::log_error (
            "String parsing failed in handleValue for objRef: %s",
            objRef.c_str ());
        return;
    }

    std::string extracted;
    std::string fcString;
    FunctionalConstraint fcValue = IEC61850_FC_NONE;

    if (secondDotPos != std::string::npos)
    {
        extracted
            = objRef.substr (secondDotPos + 1, bracketPos - secondDotPos - 1);
    }

    fcString
        = objRef.substr (bracketPos + 1, objRef.find (']') - bracketPos - 1);
    fcValue = stringToFunctionalConstraint (fcString);
    if (secondDotPos != std::string::npos)
    {
        objRef.erase (secondDotPos);
    }
    else
    {
        objRef.erase (bracketPos);
    }

    const std::shared_ptr<DataExchangeDefinition> def
        = m_config->getExchangeDefinitionByObjRef (objRef);

    if (!def)
    {
        Iec61850Utility::log_debug ("No exchange definition found for %s",
                                    objRef.c_str ());
        return;
    }

    CDCTYPE typeId = def->cdcType;

    labels.push_back (def->label);

    m_handleMonitoringData (def->objRef, datapoints, def->label, typeId,
                            mmsValue, extracted, fcValue, timestamp);
    Iec61850Utility::log_debug ("Send %s",
                                datapoints[0]->toJSONProperty ().c_str ());
    sendData (datapoints, labels);
}

void
IEC61850Client::m_handleMonitoringData (
    const std::string& objRef, std::vector<Datapoint*>& datapoints,
    const std::string& label, CDCTYPE type, MmsValue* mmsVal,
    const std::string& attribute, FunctionalConstraint fc, uint64_t timestamp)
{
    if (!m_active_connection)
    {
        Iec61850Utility::log_error ("No active connection");
        return;
    }

    IedClientError error;
    MmsValue* mmsvalue
        = mmsVal
              ? mmsVal
              : m_active_connection->readValue (&error, objRef.c_str (), fc);

    if (!mmsvalue)
    {
        logIedClientError (error, "Get MmsValue " + objRef);
        return;
    }

    auto def = m_config->getExchangeDefinitionByObjRef (objRef);
    if (!def || !def->spec)
    {
        Iec61850Utility::log_error ("Invalid definition/spec for %s",
                                    objRef.c_str ());
        cleanUpMmsValue (mmsVal, mmsvalue);
        return;
    }

    Quality quality = extractQuality (mmsvalue, def->spec, attribute);
    uint64_t ts;

    if (!mmsVal)
        ts = extractTimestamp (mmsvalue, def->spec, attribute);
    else
        ts = timestamp;

    if (!processDatapoint (type, datapoints, label, objRef, mmsvalue,
                           def->spec, quality, ts, attribute))
    {
        Iec61850Utility::log_error ("Error processing datapoint %s",
                                    objRef.c_str ());
    }

    cleanUpMmsValue (mmsVal, mmsvalue);
}

Quality
IEC61850Client::extractQuality (MmsValue* mmsvalue,
                                MmsVariableSpecification* varSpec,
                                const std::string& attribute)
{
    MmsValue const* qualityMms
        = MmsValue_getSubElement (mmsvalue, varSpec, (char*)"q");
    return (!qualityMms && attribute != "q")
               ? QUALITY_VALIDITY_GOOD
               : Quality_fromMmsValue (qualityMms);
}

uint64_t
IEC61850Client::extractTimestamp (MmsValue* mmsvalue,
                                  MmsVariableSpecification* varSpec,
                                  const std::string& attribute)
{
    MmsValue const* timestampMms
        = MmsValue_getSubElement (mmsvalue, varSpec, (char*)"t");
    return (!timestampMms && attribute != "t")
               ? PivotTimestamp::GetCurrentTimeInMs ()
               : MmsValue_getUtcTimeInMs (timestampMms);
}

bool
IEC61850Client::processDatapoint (
    CDCTYPE type, std::vector<Datapoint*>& datapoints,
    const std::string& label, const std::string& objRef, MmsValue* mmsvalue,
    MmsVariableSpecification* varSpec, Quality quality, uint64_t timestamp,
    const std::string& attribute)
{
    switch (type)
    {
    case SPC:
    case SPS:
        return processBooleanType (datapoints, label, objRef, mmsvalue,
                                   varSpec, quality, timestamp, attribute,
                                   "stVal");
    case BSC:
        return processBSCType (datapoints, label, objRef, mmsvalue, varSpec,
                               quality, timestamp, attribute, "valWTr");
    case MV:
        return processAnalogType (datapoints, label, objRef, mmsvalue, varSpec,
                                  quality, timestamp, attribute, "mag");
    case APC:
        return processAnalogType (datapoints, label, objRef, mmsvalue, varSpec,
                                  quality, timestamp, attribute, "mxVal");
    case ENS:
    case INS:
    case DPS:
    case DPC:
    case INC:
        return processIntegerType (datapoints, label, objRef, mmsvalue,
                                   varSpec, quality, timestamp, attribute,
                                   "stVal");
    default:
        return false;
    }
}

void
IEC61850Client::cleanUpMmsValue (MmsValue* originalMmsVal,
                                 MmsValue* usedMmsVal)
{
    if (usedMmsVal && !originalMmsVal)
        MmsValue_delete (usedMmsVal);
}

bool
IEC61850Client::processBooleanType (
    std::vector<Datapoint*>& datapoints, const std::string& label,
    const std::string& objRef, MmsValue* mmsvalue,
    MmsVariableSpecification* varSpec, Quality quality, uint64_t timestamp,
    const std::string& attribute, const char* elementName)
{
    MmsValue const* element
        = MmsValue_getSubElement (mmsvalue, varSpec, (char*)elementName);
    if (!element)
    {
        if (attribute == elementName)
        {
            element = mmsvalue;
        }
        else
        {
            Iec61850Utility::log_error ("No %s found %s", elementName,
                                        objRef.c_str ());
            return false;
        }
    }
    bool value = MmsValue_getBoolean (element);
    datapoints.push_back (
        m_createDatapoint (label, objRef, (long)value, quality, timestamp));
    return true;
}

bool
IEC61850Client::processBSCType (std::vector<Datapoint*>& datapoints,
                                const std::string& label,
                                const std::string& objRef, MmsValue* mmsvalue,
                                MmsVariableSpecification* varSpec,
                                Quality quality, uint64_t timestamp,
                                const std::string& attribute,
                                const char* elementName)
{
    MmsValue* element
        = MmsValue_getSubElement (mmsvalue, varSpec, (char*)elementName);
    if (!element)
    {
        if (attribute == elementName)
        {
            element = mmsvalue;
        }
        else
        {
            Iec61850Utility::log_error ("No %s found %s", elementName,
                                        objRef.c_str ());
            return false;
        }
    }

    varSpec = MmsVariableSpecification_getChildSpecificationByName (
        varSpec, elementName, nullptr);
    MmsValue const* posVal
        = MmsValue_getSubElement (element, varSpec, (char*)"posVal");
    MmsValue const* transInd
        = MmsValue_getSubElement (element, varSpec, (char*)"transInd");

    if (!posVal || !transInd)
    {
        Iec61850Utility::log_error ("Missing components in %s %s", elementName,
                                    objRef.c_str ());
        return false;
    }

    long value = MmsValue_toInt32 (posVal);
    bool transIndVal = MmsValue_getBoolean (transInd);
    long combinedValue = (value << 1) | (long)transIndVal;
    datapoints.push_back (
        m_createDatapoint (label, objRef, combinedValue, quality, timestamp));
    return true;
}

bool
IEC61850Client::processAnalogType (
    std::vector<Datapoint*>& datapoints, const std::string& label,
    const std::string& objRef, MmsValue* mmsvalue,
    MmsVariableSpecification* varSpec, Quality quality, uint64_t timestamp,
    const std::string& attribute, const char* elementName)
{
    MmsValue* element
        = MmsValue_getSubElement (mmsvalue, varSpec, (char*)elementName);
    if (!element)
    {
        if (attribute == elementName)
        {
            element = mmsvalue;
        }
        else
        {
            Iec61850Utility::log_error ("No %s found %s", elementName,
                                        objRef.c_str ());
            return false;
        }
    }

    varSpec = MmsVariableSpecification_getChildSpecificationByName (
        varSpec, elementName, nullptr);
    MmsValue* f = MmsValue_getSubElement (element, varSpec, (char*)"f");

    if (f)
    {
        double value = MmsValue_toFloat (f);
        datapoints.push_back (
            m_createDatapoint (label, objRef, value, quality, timestamp));
        return true;
    }

    MmsValue* i = MmsValue_getSubElement (element, varSpec, (char*)"i");
    if (i)
    {
        long value = MmsValue_toInt32 (i);
        datapoints.push_back (
            m_createDatapoint (label, objRef, value, quality, timestamp));
        return true;
    }

    Iec61850Utility::log_error ("No analog value found %s", objRef.c_str ());
    return false;
}

bool
IEC61850Client::processIntegerType (
    std::vector<Datapoint*>& datapoints, const std::string& label,
    const std::string& objRef, MmsValue* mmsvalue,
    MmsVariableSpecification* varSpec, Quality quality, uint64_t timestamp,
    const std::string& attribute, const char* elementName)
{
    MmsValue const* element
        = MmsValue_getSubElement (mmsvalue, varSpec, (char*)elementName);
    if (!element)
    {
        if (attribute == elementName)
        {
            element = mmsvalue;
        }
        else
        {
            Iec61850Utility::log_error ("No %s found %s", elementName,
                                        objRef.c_str ());
            return false;
        }
    }
    long value = MmsValue_toInt32 (element);
    datapoints.push_back (
        m_createDatapoint (label, objRef, value, quality, timestamp));
    return true;
}

template <class T>
Datapoint*
IEC61850Client::m_createDatapoint (const std::string& label,
                                   const std::string& objRef, T value,
                                   Quality quality, uint64_t timestamp)
{
    auto def = m_config->getExchangeDefinitionByLabel (label);

    Datapoint* pivotDp = createDp ("PIVOT");

    auto root = (PIVOTROOT)getRootFromCDC (def->cdcType);

    Datapoint* rootDp = addElement (pivotDp, rootToStrMap.at (root));
    addElementWithValue (rootDp, "ComingFrom", (std::string) "iec61850");
    addElementWithValue (rootDp, "Identifier", (std::string)def->label);
    Datapoint* cdcDp = addElement (rootDp, cdcToStrMap.at (def->cdcType));

    addValueDp (cdcDp, def->cdcType, value);
    addQualityDp (cdcDp, quality);
    addTimestampDp (cdcDp, timestamp);

    return pivotDp;
}

void
IEC61850Client::addQualityDp (Datapoint* cdcDp, Quality quality) const
{
    Datapoint* qualityDp = addElement (cdcDp, "q");
    addElementWithValue (qualityDp, "test",
                         (long)Quality_isFlagSet (&quality, QUALITY_TEST));

    Validity val = Quality_getValidity (&quality);
    if (val == QUALITY_VALIDITY_GOOD)
    {
        addElementWithValue (qualityDp, "Validity", (std::string) "good");
    }
    else if (val == QUALITY_VALIDITY_INVALID)
    {
        addElementWithValue (qualityDp, "Validity", (std::string) "invalid");
    }
    else if (val == QUALITY_VALIDITY_RESERVED)
    {
        addElementWithValue (qualityDp, "Validity", (std::string) "reserved");
    }
    else if (val == QUALITY_VALIDITY_QUESTIONABLE)
    {
        addElementWithValue (qualityDp, "Validity",
                             (std::string) "questionable");
    }

    Datapoint* detailQualityDp = addElement (qualityDp, "DetailQuality");

    if (Quality_isFlagSet (&quality, QUALITY_DETAIL_OVERFLOW))
    {
        addElementWithValue (detailQualityDp, "overflow", (long)true);
    }
    if (Quality_isFlagSet (&quality, QUALITY_DETAIL_OLD_DATA))
    {
        addElementWithValue (detailQualityDp, "oldData", (long)true);
    }
    if (Quality_isFlagSet (&quality, QUALITY_DETAIL_OUT_OF_RANGE))
    {
        addElementWithValue (detailQualityDp, "outOfRange", (long)true);
    }

    if (Quality_isFlagSet (&quality, QUALITY_OPERATOR_BLOCKED))
    {
        addElementWithValue (qualityDp, "operatorBlocked", (long)true);
    }

    if (Quality_isFlagSet (&quality, QUALITY_SOURCE_SUBSTITUTED))
    {
        addElementWithValue (qualityDp, "Source", (std::string) "substituted");
    }
}

void
IEC61850Client::addTimestampDp (Datapoint* cdcDp, uint64_t timestampMs) const
{
    std::unique_ptr<PivotTimestamp> ts (new PivotTimestamp (timestampMs));
    Datapoint* tsDp = addElement (cdcDp, "t");
    addElementWithValue (tsDp, "SecondSinceEpoch",
                         (long)ts->SecondSinceEpoch ());
    addElementWithValue (tsDp, "FractionOfSecond",
                         (long)ts->FractionOfSecond ());
}

template <class T>
void
IEC61850Client::addValueDp (Datapoint* cdcDp, CDCTYPE type, T value) const
{
    switch (type)
    {
    case SPC:
    case INC:
    case ENS:
    case INS:
    case SPS: {
        addElementWithValue (cdcDp, "stVal", (long)value);
        break;
    }
    case DPC:
    case DPS: {
        auto valueInt = (long)value;
        std::string stVal;
        if (valueInt == 0)
            stVal = "intermediate-state";
        else if (valueInt == 1)
            stVal = "off";
        else if (valueInt == 2)
            stVal = "on";
        else if (valueInt == 3)
            stVal = "bad-state";
        addElementWithValue (cdcDp, "stVal", (std::string)stVal);
        break;
    }
    case APC:
    case MV: {
        Datapoint* magDp = addElement (cdcDp, type == MV ? "mag" : "mxVal");
        if (std::is_same<T, double>::value)
        {
            addElementWithValue (magDp, "f", (double)value);
        }
        else if (std::is_same<T, long>::value)
        {
            addElementWithValue (magDp, "i", (long)value);
        }
        else
        {
            Iec61850Utility::log_error ("Invalid mag data type");
        }
        break;
    }
    case BSC: {
        Datapoint* valWtrDp = addElement (cdcDp, "valWtr");
        addElementWithValue (valWtrDp, "posVal", (long)value >> 1);
        addElementWithValue (valWtrDp, "transInd", (long)value & 1);
        break;
    }
    default: {
        Iec61850Utility::log_error ("Invalid cdcType %d", type);
        break;
    }
    }
}

bool
IEC61850Client::handleOperation (Datapoint* operation)
{

    Datapoint* identifierDp = getChild (operation, "Identifier");

    if (!identifierDp
        || identifierDp->getData ().getType () != DatapointValue::T_STRING)
    {
        Iec61850Utility::log_warn ("Operation has no identifier");
        return false;
    }

    std::string id = getValueStr (identifierDp);

    const std::shared_ptr<DataExchangeDefinition> def
        = m_config->getExchangeDefinitionByPivotId (id);

    if (!def)
    {
        Iec61850Utility::log_warn (
            "No exchange definition found for pivot id %s", id.c_str ());
        return false;
    }

    std::string label = def->label;

    std::string objRef = def->objRef;

    Datapoint* cdcDp = getCdc (operation);

    if (!cdcDp)
    {
        Iec61850Utility::log_error ("Operation has no cdc");
        return false;
    }

    Datapoint* valueDp = getChild (cdcDp, "ctlVal");

    if (!valueDp)
    {
        Iec61850Utility::log_error ("Operation has no value");
        return false;
    }

    DatapointValue value = valueDp->getData ();

    bool res = m_active_connection->operate (objRef, value);

    m_outstandingCommands[label] = operation;

    return res;
}

void
IEC61850Client::sendCommandAck (const std::string& label, ControlModel mode,
                                bool terminated)
{
    if (m_outstandingCommands.empty ())
    {
        Iec61850Utility::log_error ("No outstanding commands");
        return;
    }

    auto it = m_outstandingCommands.find (label);
    if (it == m_outstandingCommands.end ())
    {
        Iec61850Utility::log_error (
            "No outstanding command with label %s found", label.c_str ());
        return;
    }

    Datapoint* pivotRoot = createDp ("PIVOT");
    Datapoint* command
        = addElementWithValue (pivotRoot, "GTIC", it->second->getData ());
    int cot = terminated ? 10 : 7;
    Datapoint* causeDp = nullptr;
    causeDp = getChild (command, "Cause");
    if (causeDp)
    {
        Datapoint* stVal = getChild (causeDp, "stVal");
        if (!stVal)
        {
            Iec61850Utility::log_error ("Cause dp has no stVal");
            delete pivotRoot;
            return;
        }
        stVal->getData ().setValue ((long)cot);
    }
    else
    {
        causeDp = addElement (command, "Cause");
        addElementWithValue (causeDp, "stVal", (long)cot);
    }

    std::vector<Datapoint*> datapoints;
    std::vector<std::string> labels;
    labels.push_back (label);
    datapoints.push_back (pivotRoot);
    sendData (datapoints, labels);

    if (terminated
        || (!terminated
            && (mode == CONTROL_MODEL_SBO_NORMAL
                || mode == CONTROL_MODEL_DIRECT_NORMAL)))
    {
        delete m_outstandingCommands[label];
        m_outstandingCommands.erase (label);
    }
}
