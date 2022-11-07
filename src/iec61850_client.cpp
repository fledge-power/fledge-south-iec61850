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

#include "./iec61850_client.h"

// C++ headers
#include <memory>
#include <vector>

// local library
#include "./iec61850.h"
#include "./iec61850_client_connection.h"

constexpr const uint32_t SECOND_IN_MILLISEC = 1000;
constexpr const uint32_t RECONNECTION_FREQUENCY_IN_HERTZ = 1;
constexpr const uint32_t DEMO_MMS_READ_FREQUENCY_IN_HERTZ = 2;

IEC61850Client::IEC61850Client(IEC61850 *iec61850,
                               const ServerConnectionParameters &connectionParam,
                               const ExchangedData &exchangedData)
    : m_connectionParam(connectionParam),
      m_exchangedData(exchangedData),
      m_iec61850(iec61850)
{
    /** Complete the DA path, with the name of the server */
    m_exchangedData.daPath = m_connectionParam.serverName +
                             m_exchangedData.daPathWithoutServerName;


    m_clientId = IEC61850ClientConfig::buildKey(m_connectionParam);

    Logger::getLogger()->debug("IEC61850Client: constructor %s",
                               m_clientId.c_str());
}

IEC61850Client::~IEC61850Client()
{
    Logger::getLogger()->debug("IEC61850Client: destructor %s",
                               m_clientId.c_str());
    stop();  // ensure a correct shutdown, if 'stop' order was missing
}

void IEC61850Client::start()
{
    m_backgroundLaunchThread = std::thread(&IEC61850Client::launch, this);
}

void IEC61850Client::stop()
{
    m_stopOrder = true;
    if (m_backgroundLaunchThread.joinable()) {
        m_backgroundLaunchThread.join();
    }

    // Stop the demo
    stopDemo();
    destroyConnection();
}

void IEC61850Client::launch()
{
    initializeConnection();

    /** Make subscriptions */
    // TODO

    /** Start application loop */
    // Start the demo
    startDemo();
}

void IEC61850Client::initializeConnection()
{
    do {
        Logger::getLogger()->debug("IEC61850Client: init connection (%s)",
                                   m_clientId.c_str());
        destroyConnection();
        createConnection();

        // Wait connection establishment
        std::chrono::milliseconds timespan(SECOND_IN_MILLISEC / RECONNECTION_FREQUENCY_IN_HERTZ);
        std::this_thread::sleep_for(timespan);

    } while ( (! m_stopOrder) &&
            (! m_connection->isConnected()));
}

void IEC61850Client::createConnection()
{
    // Preconditions
    if (m_connection) {
        Logger::getLogger()->info("IEC61850Client: connection is already created (%s)",
                                  m_clientId.c_str());
        return;
    }

    Logger::getLogger()->debug("IEC61850Client: create connection (%s)",
                               m_clientId.c_str());
    m_connection = std::make_unique<IEC61850ClientConnection>(m_connectionParam);
}

void IEC61850Client::destroyConnection()
{
    if (! m_connection) {
        Logger::getLogger()->info("IEC61850Client: connection does not exist (%s)",
                                  m_clientId.c_str());
        return;
    }

    Logger::getLogger()->debug("IEC61850Client: destroy connection (%s)",
                               m_clientId.c_str());
    m_connection.reset(nullptr);
}


template <typename T>
Datapoint *IEC61850Client::createDatapoint(const std::string &dataName,
        T primitiveTypeValue)
{
    DatapointValue value = DatapointValue(primitiveTypeValue);
    /** Dynamic allocation with raw pointer: Fledge core will deallocate it */
    auto *datapoint = new Datapoint(dataName, value);  // NOSONAR
    return datapoint;
}


void IEC61850Client::sendData(Datapoint *dataPoint)
{
    std::vector<Datapoint *> points(0);
    points.push_back(dataPoint);
    m_iec61850->ingest(points);
}

Datapoint *IEC61850Client::convertMmsToDatapoint(std::shared_ptr<Mms> mms)
{
    // Precondition
    if (nullptr == mms) {
        return nullptr;
    }

    Datapoint *datapoint = nullptr;
    /* Test the Mms value type */
    const MmsValue *mmsValue = mms->getMmsValue();

    switch (MmsValue_getType(mmsValue))  {
        case MMS_FLOAT:
            datapoint = createDatapoint("MMS_FLOAT",
                                        MmsValue_toFloat(mmsValue));
            break;

        case MMS_BOOLEAN: {
            bool boolValue = MmsValue_getBoolean(mmsValue);
            datapoint = createDatapoint("MMS_BOOLEAN",
                                        static_cast<int64_t>(boolValue ? 1 : 0));
            break;
        }

        case MMS_INTEGER:
            datapoint = createDatapoint("MMS_INTEGER",
                                        static_cast<int64_t>(MmsValue_toInt32(mmsValue)));
            break;

        case MMS_VISIBLE_STRING:
            // TODO fix 'MmsValue_toString()' signature in libiec61850
            // TODO use: createDatapoint("MMS_VISIBLE_STRING", MmsValue_toString(mmsValue));
            datapoint = createDatapoint("MMS_VISIBLE_STRING",
                                        MmsValue_toString(const_cast<MmsValue *>(mmsValue)));
            break;

        case MMS_UNSIGNED:
            datapoint = createDatapoint("MMS_UNSIGNED",
                                        static_cast<int64_t>(MmsValue_toUint32(mmsValue)));
            break;

        case MMS_OCTET_STRING: {
            // TODO fix 'MmsValue_getOctetStringBuffer()' signature in libiec61850
            // TODO use: MmsValue_getOctetStringBuffer(mmsValue);
            uint8_t *mms_string_buffer = MmsValue_getOctetStringBuffer(const_cast<MmsValue *>(mmsValue));
            std::string sval(reinterpret_cast<char *>(mms_string_buffer),  // only 'string of char' is supported
                             MmsValue_getOctetStringSize(mmsValue));
            datapoint = createDatapoint("MMS_OCTET_STRING", sval);
            break;
        }

        case MMS_DATA_ACCESS_ERROR:
            Logger::getLogger()->warn("MMS access error, please reconfigure");
            break;

        default :
            Logger::getLogger()->warn("Unsupported MMS data type");
            break;
    }

    return datapoint;
}

// Demo section

void IEC61850Client::startDemo()
{
    m_isDemoLoopActivated = true;
    m_demoLoopThread = std::thread(&IEC61850Client::readMmsLoop, this);
}

void IEC61850Client::stopDemo()
{
    // Preconditions
    if (false == m_isDemoLoopActivated) {
        return;
    }

    m_isDemoLoopActivated = false;
    m_demoLoopThread.join();
}

void IEC61850Client::readMmsLoop()
{
    // Preconditions
    if (! m_connection) {
        Logger::getLogger()->warn("IEC61850Client: Connection object is null");
    }

    while (m_isDemoLoopActivated) {
        if (m_connection->isConnected()) {
            if (m_connection->isNoError()) {
                readAndExportMms();
            } else {
                Logger::getLogger()->info("No data to read");
            }

            std::chrono::milliseconds timespan(SECOND_IN_MILLISEC / DEMO_MMS_READ_FREQUENCY_IN_HERTZ);
            std::this_thread::sleep_for(timespan);
        } else {
            initializeConnection();
        }
    }
}

void IEC61850Client::readAndExportMms()
{
    /* read an analog measurement value from server */
    std::shared_ptr<Mms> mms;
    mms = m_connection->readMms(m_exchangedData.daPath,
            m_exchangedData.fcName);

    if (mms != nullptr) {
        Datapoint *datapoint = convertMmsToDatapoint(mms);
        sendData(datapoint);
    }
}
