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

class IEC61850Client;

class IEC61850
{
    public:
        IEC61850(const char *ip,
                 const uint16_t port,
                 const std::string &iedModel,
                 const std::string &logicalNode,
                 const std::string &logicalDevice,
                 const std::string &cdc,
                 const std::string &attribute,
                 const std::string &fc);
        ~IEC61850() = default;

        void setIp(const char *ip_address);
        void setPort(uint16_t port);
        void setAssetName(const std::string &name);
        void setLogicalDevice(const std::string &logicaldevice_name);;
        void setLogicalNode(const std::string &logicalnode_name);
        void setAttribute(const std::string &attribute_name);
        void setFc(const std::string &fc_name);

        void start();
        void stop();
        void ingest(std::vector<Datapoint *>  points);
        void registerIngest(void *data, void (*cb)(void *, Reading))
        {
            m_ingest = cb;
            m_data = data;
        }

        void setModel(const std::string &model);
        void setCdc(const std::string &CDC);

        void loop();
        std::mutex loopLock;
        std::atomic<bool> loopActivated{};
        std::thread loopThread;

    private:
        void readMmsLoop();
        void exportMmsValue(MmsValue *value);

        std::string         m_asset;
        std::string         m_ip;
        uint16_t            m_port;
        std::string         m_logicaldevice;
        std::string         m_logicalnode;
        std::string         m_iedmodel;
        std::string         m_cdc;
        std::string         m_attribute;
        std::string         m_fc;
        std::string         m_goto;
        IedConnection       m_iedconnection;
        IedClientError      m_error;
        void                (*m_ingest)(void *, Reading) {}; // NOLINT
        void                *m_data{};

        IEC61850Client      *m_client;
};


class IEC61850Client
{
    public :

        // Send the MMS data from the South plugin to Fledge

        explicit IEC61850Client(IEC61850 *iec61850) : m_iec61850(iec61850) {}

        template <typename T>
        void sendData(const std::string &dataname, T a)
        {
            DatapointValue value = DatapointValue(a);
            std::vector<Datapoint *> points;
            std::string name = dataname;
            points.push_back(new Datapoint(name, value));
            m_iec61850->ingest(points);
        }

    private:
        IEC61850    *m_iec61850;
};

#endif  // INCLUDE_IEC61850_H_
