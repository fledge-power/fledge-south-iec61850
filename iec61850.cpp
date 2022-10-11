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


IEC61850::IEC61850(const char *ip,
                   const uint16_t port,
                   const std::string &iedModel,
                   const std::string &logicalNode,
                   const std::string &logicalDevice,
                   const std::string &cdc,
                   const std::string &attribute,
                   const std::string &fc):
    m_ip(ip),
    m_port(port),
    m_logicaldevice(logicalDevice),
    m_logicalnode(logicalNode),
    m_iedmodel(iedModel),
    m_cdc(cdc),
    m_attribute(attribute),
    m_fc(fc),
    m_goto(""),
    m_error(IED_ERROR_OK),
    m_client(nullptr)
{
    m_iedconnection = nullptr;
}

// Set the IP of the 61850 server
void IEC61850::setIp(const char *ip)
{
    if (strlen(ip) > 1) {
        /* Ip set to the ip given by user */
        m_ip = ip;
    } else {
        /* Default IP if entry null*/
        m_ip = "127.0.0.1";
    }
}

// Set the port of the 61850 server
void IEC61850::setPort(uint16_t port)
{
    if (port > 0) {
        /* port set to the port given by the user*/
        m_port = port;
    } else {
        /* Default port for IEC61850 */
        m_port = 8102;
    }
}

// Set the name of the asset
void IEC61850::setAssetName(const std::string &name)
{
    m_asset = name;
}

// Set the name of the logical device
void IEC61850::setLogicalDevice(const std::string &logicaldevice_name)
{
    m_logicaldevice = logicaldevice_name;
}

// Set the name of the logical node
void IEC61850::setLogicalNode(const std::string &logicalnode_name)
{
    m_logicalnode = logicalnode_name;
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
void IEC61850::setAttribute(const std::string &attribute_name)
{
    m_attribute = attribute_name;
}

// Set the name of the functional constraint
void IEC61850::setFc(const std::string &fc_name)
{
    m_fc = fc_name;
}

void IEC61850::start()
{
    Logger::getLogger()->info("Plugin started");
    /* Creating the client for fledge */
    m_client = new IEC61850Client(this);
    /* The type of Data class */
    m_goto = m_iedmodel + m_logicaldevice +
             "/" +
             m_logicalnode +
             "." +
             m_cdc +
             "." +
             m_attribute;
    loopActivated = true;
    loopThread = std::thread(&IEC61850::loop, this);
}

void IEC61850::loop()
{
    /* Retry if connection lost */
    while (loopActivated) {
        m_iedconnection = IedConnection_create();
        /* Connect with the object connection reference */
        IedConnection_connect(m_iedconnection, &m_error, m_ip.c_str(), m_port);

        if (nullptr != m_iedconnection) {
            readMmsLoop();
        }

        std::chrono::milliseconds timespan(500);
        std::this_thread::sleep_for(timespan);
    }
}

void IEC61850::readMmsLoop()
{
    while ( (IedConnection_getState(m_iedconnection) == IED_STATE_CONNECTED) &&
            loopActivated) {
        std::unique_lock<std::mutex> guard2(loopLock);

        if (m_error == IED_ERROR_OK) {
            /* read an analog measurement value from server */
            MmsValue *value = IedConnection_readObject(m_iedconnection,
                              &m_error,
                              m_goto.c_str(),
                              FunctionalConstraint_fromString(m_fc.c_str()));
            exportMmsValue(value);
        } else {
            Logger::getLogger()->info("No data to read");
        }

        guard2.unlock();
        std::chrono::milliseconds timespan(100);
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

        case (MMS_BOOLEAN):
            m_client->sendData("MMS_BOOLEAN",
                               int64_t(MmsValue_getBoolean(value) ? 1 : 0));
            break;

        case (MMS_INTEGER):
            m_client->sendData("MMS_INTEGER", int64_t(MmsValue_toInt32(value)));
            break;

        case (MMS_VISIBLE_STRING) :
            m_client->sendData("MMS_VISIBLE_STRING", MmsValue_toString(value));
            break;

        case (MMS_UNSIGNED):
            m_client->sendData("MMS_UNSIGNED",
                               int64_t(MmsValue_toUint32(value)));
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
    // Stop the MMS reader thread
    loopActivated = false;
    loopThread.join();

    if (m_iedconnection != nullptr && IedConnection_getState(m_iedconnection)) {
        /* Close the connection */
        IedConnection_close(m_iedconnection);
        /* Destroy the connection instance after closing it */
        IedConnection_destroy(m_iedconnection);
    }

    if (nullptr != m_client) {
        delete m_client;
        m_client = nullptr;
    }
}

void IEC61850::ingest(std::vector<Datapoint *> points)
{
    /* Creating the name of the type of data */
    std::string asset = points[0]->getName();
    /* Callback function used after receiving data */
    (*m_ingest)(m_data, Reading(asset, points));
}
