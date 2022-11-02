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

IEC61850::IEC61850()
    : ClientGatewayInterface(),
      FledgeProxyInterface()
{
    m_config = std::make_shared<IEC61850ClientConfig>();
}

void IEC61850::setConfig(const ConfigCategory &config) const
{
    if (m_config) {
        m_config->importConfig(config);
    } else {
        Logger::getLogger()->warn("IEC61850ClientConfig object is null");
    }
}

std::string IEC61850::getLogMinLevel() const
{
    if (m_config) {
        return m_config->logMinLevel;
    } else {
        return "info";
    }
}

void IEC61850::start()
{
    Logger::getLogger()->info("Plugin started");

    /* Create the IEC61850 clients */
    for(auto &serverConfig: m_config->serverConfigDict) {
        std::string key = serverConfig.first;

        m_clients[key] = std::make_unique<IEC61850Client>(this,
                                                          serverConfig.second,
                                                          m_config->exchangedData);
        m_clients[key]->start();
    }
}

void IEC61850::stop()
{
    for(auto &client: m_clients) {
        client.second->stop();
    }

    m_clients.clear();
}

void IEC61850::ingest(std::vector<Datapoint *> points)
{
    std::unique_lock<std::mutex> ingestGuard(m_ingestMutex);

    if (m_ingest_callback) {
        /* Creating the name of the type of data */
        std::string asset = points[0]->getName();
        /* Callback function used after receiving data */
        (*m_ingest_callback)(m_data, Reading(asset, points));
    }
}
