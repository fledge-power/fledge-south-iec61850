#ifndef INCLUDE_IEC61850_H_
#define INCLUDE_IEC61850_H_

/*
 * Fledge IEC 61850 south plugin.
 *
 * Copyright (c) 2020, RTE (https://www.rte-france.com)
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Estelle Chigot, Lucas Barret
 */

#include <thread>  // NOLINT
#include <mutex>   // NOLINT
#include <atomic>
#include <string>
#include <vector>
#include <memory>

// libiec61850 headers
#include <libiec61850/iec61850_model.h>
#include <libiec61850/iec61850_common.h>
#include <libiec61850/iec61850_client.h>
#include <libiec61850/mms_common.h>
#include <libiec61850/mms_types.h>
#include <libiec61850/mms_value.h>

// Fledge headers
#include <reading.h>
#include <logger.h>

constexpr const uint16_t DEFAULT_MMS_PORT = 8102;
const char *const DEFAULT_IED_IP_ADDRESS = "127.0.0.1";

class IEC61850Client;

class IEC61850
{
    public:
        IEC61850(std::string ipAddress,
                 uint16_t mmsPort,
                 std::string iedModel,
                 std::string logicalNode,
                 std::string logicalDevice,
                 std::string cdc,
                 std::string attribute,
                 std::string fonctionalConstraint);
        ~IEC61850();

        /** Unavailable copy constructor */
        IEC61850(const IEC61850 &) = delete;
        /** Unavailable copy assignment operator */
        IEC61850 &operator = (const IEC61850 &) = delete;
        /** Unavailable move constructor */
        IEC61850(IEC61850 &&) = delete;
        /** Unavailable move assignment operator */
        IEC61850 &operator = (IEC61850 &&) = delete;

        void setIedIpAddress(const std::string &ipAddress);
        void setMmsPort(uint16_t mmsPort);
        void setAssetName(const std::string &name);
        void setLogicalDevice(const std::string &logicalDeviceName);;
        void setLogicalNode(const std::string &logicalNodeName);
        void setAttribute(const std::string &attributeName);
        void setFc(const std::string &fcName);

        void start();
        void stop();
        void ingest(std::vector<Datapoint *>  points);
        void registerIngest(void *data, void (*ingest_callback)(void *, Reading))
        {
            m_ingest_callback = ingest_callback;
            m_data = data;
        }

        void setModel(const std::string &model);
        void setCdc(const std::string &CDC);

        void loop();

    private:
        void readMmsLoop();
        void exportMmsValue(MmsValue *value);

        std::string         m_assetName;
        std::string         m_ipAddress;
        uint16_t            m_mmsPort;
        std::string         m_logicalDeviceName;
        std::string         m_logicalNodeName;
        std::string         m_iedmodel;
        std::string         m_cdc;
        std::string         m_attribute;
        std::string         m_fc;
        std::string         m_goto;
        IedConnection       m_iedConnection = nullptr;
        IedClientError      m_networkStack_error = IED_ERROR_OK;
        void                (*m_ingest_callback)(void *, Reading) {}; // NOLINT
        void                *m_data = nullptr;

        std::atomic<bool> isLoopActivated{false};
        std::thread loopThread;

        std::mutex m_libiec61850ClientConnectionMutex; // libiec61850 thread safe?: protect the IedConnection
        std::unique_ptr<IEC61850Client> m_client;
};


class IEC61850Client
{
    public :

        // Send the MMS data from the South plugin to Fledge

        explicit IEC61850Client(IEC61850 *iec61850) : m_iec61850(iec61850) {}

        template <typename T>
        void sendData(const std::string &dataname, T primitiveTypeValue)
        {
            DatapointValue value = DatapointValue(primitiveTypeValue);
            std::vector<Datapoint *> points(0);
            points.push_back(new Datapoint(dataname, value));
            m_iec61850->ingest(points);
        }

    private:
        IEC61850    *m_iec61850;
};

#endif  // INCLUDE_IEC61850_H_
