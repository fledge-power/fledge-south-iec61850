/*
 * Fledge IEC 61850 south plugin.
 *
 * Copyright (c) 2020, RTE (https://www.rte-france.com)
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Estelle Chigot, Lucas Barret
 *
 * Contributor: Colin Constans
 */

#include "./iec61850.h"

constexpr const uint32_t MMS_READ_FREQUENCY_IN_HERTZ = 10;
constexpr const uint32_t WAIT_BEFORE_NEW_CONNECTION_TRY_IN_MILLISEC = 500;

IEC61850::IEC61850(std::string ipAddress,
                   uint16_t mmsPort,
                   std::string iedModel,
                   std::string logicalNodeName,
                   std::string logicalDeviceName,
                   std::string cdc,
                   std::string attribute,
                   std::string fonctionalConstraint):
    m_ipAddress(std::move(ipAddress)),
    m_mmsPort(mmsPort),
    m_logicalDeviceName(std::move(logicalDeviceName)),
    m_logicalNodeName(std::move(logicalNodeName)),
    m_iedmodel(std::move(iedModel)),
    m_cdc(std::move(cdc)),
    m_attribute(std::move(attribute)),
    m_fc(std::move(fonctionalConstraint)),
    m_client(nullptr)
{
}

IEC61850::~IEC61850()
{
    stop();
}

// Set the IP of the 61850 server
void IEC61850::setIedIpAddress(const std::string &ipAddress)
{
    if (ipAddress.empty()) {
        /* Default IP if entry null*/
        m_ipAddress = DEFAULT_IED_IP_ADDRESS;
    } else {
        m_ipAddress = ipAddress;
    }
}

// Set the MMS port of the 61850 server
void IEC61850::setMmsPort(uint16_t mmsPort)
{
    if (mmsPort > 0) {
        /* port set to the port given by the user*/
        m_mmsPort = mmsPort;
    } else {
        /* Default MMS port for IEC61850 */
        m_mmsPort = DEFAULT_MMS_PORT;
    }
}

// Set the name of the asset
void IEC61850::setAssetName(const std::string &assetName)
{
    m_assetName = assetName;
}

// Set the name of the logical device
void IEC61850::setLogicalDevice(const std::string &logicalDeviceName)
{
    m_logicalDeviceName = logicalDeviceName;
}

// Set the name of the logical node
void IEC61850::setLogicalNode(const std::string &logicalNodeName)
{
    m_logicalNodeName = logicalNodeName;
}

// Set the name of the IED model
void IEC61850::setModel(const std::string &model)
{
    m_iedmodel = model;
}

// Set the name of the CDC
void IEC61850::setCdc(const std::string &CDC)
{
    m_cdc = CDC;
}

// Set the name of the data attribute
void IEC61850::setAttribute(const std::string &attributeName)
{
    m_attribute = attributeName;
}

// Set the name of the functional constraint
void IEC61850::setFc(const std::string &fcName)
{
    m_fc = fcName;
}

void IEC61850::start()
{
    Logger::getLogger()->info("Plugin started");
    /* Creating the client for fledge */
    m_client = std::make_unique<IEC61850Client>(this);
    /* The type of Data class */
    m_goto = m_iedmodel + m_logicalDeviceName +
             "/" +
             m_logicalNodeName +
             "." +
             m_cdc +
             "." +
             m_attribute;
    isLoopActivated = true;
    loopThread = std::thread(&IEC61850::loop, this);
}

void IEC61850::loop()
{
    /* Retry if connection lost */
    while (isLoopActivated) {
        m_iedConnection = IedConnection_create();
        /* Connect with the object connection reference */
        IedConnection_connect(m_iedConnection,
                              &m_networkStack_error,
                              m_ipAddress.c_str(),
                              m_mmsPort);

        if (nullptr != m_iedConnection) {
            readMmsLoop();
        }

        std::chrono::milliseconds timespan(WAIT_BEFORE_NEW_CONNECTION_TRY_IN_MILLISEC);
        std::this_thread::sleep_for(timespan);
    }
}

void IEC61850::readMmsLoop()
{
    while ( (IedConnection_getState(m_iedConnection) == IED_STATE_CONNECTED) &&
            isLoopActivated) {
        std::unique_lock<std::mutex> guard2(m_libiec61850ClientConnectionMutex);

        if (m_networkStack_error == IED_ERROR_OK) {
            /* read an analog measurement value from server */
            MmsValue *value = IedConnection_readObject(m_iedConnection,
                              &m_networkStack_error,
                              m_goto.c_str(),
                              FunctionalConstraint_fromString(m_fc.c_str()));
            exportMmsValue(value);
        } else {
            Logger::getLogger()->info("No data to read");
        }

        guard2.unlock();
        std::chrono::milliseconds timespan(1000 / MMS_READ_FREQUENCY_IN_HERTZ);
        std::this_thread::sleep_for(timespan);
    }
}

void IEC61850::exportMmsValue(MmsValue *value)
{
    // Precondition
    if (nullptr == value) {
        return;
    }

    /* Test the type value */
    switch (MmsValue_getType(value))  {
        case (MMS_FLOAT) :
            m_client->sendData("MMS_FLOAT", MmsValue_toFloat(value));
            break;

        case (MMS_BOOLEAN): {
            bool boolValue = MmsValue_getBoolean(value);
            m_client->sendData("MMS_BOOLEAN",
                               static_cast<int64_t>(boolValue ? 1 : 0));
            break;
        }

        case (MMS_INTEGER):
            m_client->sendData("MMS_INTEGER",
                               static_cast<int64_t>(MmsValue_toInt32(value)));
            break;

        case (MMS_VISIBLE_STRING) :
            m_client->sendData("MMS_VISIBLE_STRING", MmsValue_toString(value));
            break;

        case (MMS_UNSIGNED):
            m_client->sendData("MMS_UNSIGNED",
                               static_cast<int64_t>(MmsValue_toUint32(value)));
            break;

        case (MMS_OCTET_STRING): {
            uint8_t *mms_string_buffer = MmsValue_getOctetStringBuffer(value);
            std::string sval( reinterpret_cast<char *>(mms_string_buffer),
                              MmsValue_getOctetStringSize(value));
            m_client->sendData("MMS_OCTET_STRING", sval);
        }
        break;

        case (MMS_DATA_ACCESS_ERROR) :
            Logger::getLogger()->info("MMS access error, please reconfigure");
            break;

        default :
            Logger::getLogger()->info("Unsupported MMS data type");
            break;
    }

    MmsValue_delete(value);
}


void IEC61850::stop()
{
    // Preconditions
    if (false == isLoopActivated) {
        return;
    }

    // Stop the MMS reader thread
    isLoopActivated = false;
    loopThread.join();

    if ( (m_iedConnection != nullptr) &&
         (IED_STATE_CLOSED != IedConnection_getState(m_iedConnection))) {
        /* Close the connection */
        IedConnection_close(m_iedConnection);
        /* Destroy the connection instance after closing it */
        IedConnection_destroy(m_iedConnection);
    }
}

void IEC61850::ingest(std::vector<Datapoint *> points)
{
    /* Creating the name of the type of data */
    std::string asset = points[0]->getName();
    /* Callback function used after receiving data */
    (*m_ingest_callback)(m_data, Reading(asset, points));
}
