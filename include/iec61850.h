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

#include <string>
#include <memory>
#include <mutex>   // NOLINT
#include <map>

// Fledge headers
#include <reading.h>
#include <logger.h>

// local library
#include "./iec61850_client_gateway_interface.h"
#include "./iec61850_fledge_proxy_interface.h"
#include "./iec61850_client.h"
#include "./iec61850_client_config.h"

class IEC61850: public ClientGatewayInterface, public FledgeProxyInterface
{
    public:
        IEC61850();
        ~IEC61850() override = default;

        /** Disable copy constructor */
        IEC61850(const IEC61850 &) = delete;
        /** Disable copy assignment operator */
        IEC61850 &operator = (const IEC61850 &) = delete;
        /** Disable move constructor */
        IEC61850(IEC61850 &&) = delete;
        /** Disable move assignment operator */
        IEC61850 &operator = (IEC61850 &&) = delete;

        void setConfig(const ConfigCategory &config) const;
        std::string getLogMinLevel() const;

        void start() override;
        void stop() override;

        void ingest(std::vector<Datapoint *>  points) override;
        void registerIngest(INGEST_DATA_TYPE data,
                            void (*ingest_cb)(INGEST_DATA_TYPE, Reading)) override
        {
            m_ingest_callback = ingest_cb;
            m_data = data;
        }


    private:

        void                (*m_ingest_callback)(void *, Reading) {}; // NOLINT
        INGEST_DATA_TYPE    m_data = nullptr;
        std::mutex          m_ingestMutex;

        std::map<std::string, std::unique_ptr<IEC61850Client>, std::less<>> m_clients;

        std::shared_ptr<IEC61850ClientConfig> m_config;
};
#endif  // INCLUDE_IEC61850_H_
