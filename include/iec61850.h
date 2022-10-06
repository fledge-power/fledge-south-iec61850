#ifndef _IEC61850MMS_H
#define _IEC61850MMS_H

/*
 * Fledge IEC 61850 south plugin.
 *
 * Copyright (c) 2020, RTE (https://www.rte-france.com)
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Estelle Chigot, Lucas Barret
 */

#include <thread>
#include <mutex>
#include <atomic>

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

using namespace std;

class IEC61850
{
    public:

        IEC61850(const char *ip,
                 uint16_t port,
                 string iedModel,
                 std::string logicalNode,
                 std::string logicalDevice,
                 std::string CDC_SAV,
                 std::string dataAttribute,
                 std::string FC);
        ~IEC61850();

        void setIp(const char *ip);
        void setPort(uint16_t port);
        void setAssetName(const std::string& name);
        void setLogicalDevice(std::string logicaldevice_name);;
        void setLogicalNode(std::string logicalnode_name);
        void setAttribute(std::string attribute_name);
        void setFc(std::string fc_name);

        void start();
        void stop();
        void ingest(std::vector<Datapoint *>  points);
        void registerIngest(void *data, void (*cb)(void *, Reading))
        {
            m_ingest = cb;
            m_data = data;
        }

        void setModel(string model);
        void setCdc(string CDC);

        void loop();
        std::mutex loopLock;
        std::atomic<bool> loopActivated{};
        thread loopThread;

    private:

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
        void                (*m_ingest)(void *, Reading){};
        void                *m_data{};

        IEC61850Client      *m_client{};
};


class IEC61850Client
{
    public :

        // Send the MMS data from the South plugin to Fledge

        explicit IEC61850Client(IEC61850 *iec61850) : m_iec61850(iec61850) {};

        void sendDatafloat(std::string dataname,float a)
        {
            DatapointValue value = DatapointValue(a);
            std::vector<Datapoint *> points;
            std::string name = dataname;
            points.push_back(new Datapoint(name,value));
            m_iec61850->ingest(points);
        }

        template <typename T>
        void sendData(std::string dataname, T a)
        {
            DatapointValue value = DatapointValue(a);
            std::vector<Datapoint *> points;
            std::string name = dataname;
            points.push_back(new Datapoint(name,value));
            m_iec61850->ingest(points);
        }

    private:
        IEC61850    *m_iec61850;
};

#endif
