#ifndef INCLUDE_IEC61850_H_
#define INCLUDE_IEC61850_H_

#include "libiec61850/iec61850_client.h"
#include <logger.h>
#include <plugin_api.h>
#include <reading.h>

// clang-format off
#include <mutex>
#include <chrono>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
// clang-format on

#include "iec61850_client_config.hpp"
#include "iec61850_client_connection.hpp"

class IEC61850Client;

class IEC61850
{
public:
    typedef void (*INGEST_CB)(void*, Reading);

    IEC61850();
    ~IEC61850();

    void setAssetName(const std::string& asset) { m_asset = asset; }
    void setJsonConfig(const std::string& stack_configuration,
                              const std::string& msg_configuration,
                              const std::string& tls_configuration);

    void start();
    void stop();

    // void ingest(Reading& reading);
    void ingest(std::string assetName, std::vector<Datapoint*>& points);
    void registerIngest(void* data, void (*cb)(void*, Reading));
    bool operation(const std::string& operation, int count,
                   PLUGIN_PARAMETER** params);

private:

    IEC61850ClientConfig* m_config;

    std::string m_asset;

protected:
    std::vector<IedConnection*> m_connections;

private:
    INGEST_CB m_ingest = nullptr;  // Callback function used to send data to south service
    void* m_data;        // Ingest function data
    IEC61850Client* m_client = nullptr;
};


class IEC61850Client
{
public:

    explicit IEC61850Client(IEC61850* iec61850, IEC61850ClientConfig* config);

    ~IEC61850Client();

    void sendData(std::vector<Datapoint*> data,
                  const std::vector<std::string> labels);

    void start();

    void stop();

private:

    IEC61850ClientConfig* m_config;
    IEC61850* m_iec61850;
};

#endif  // INCLUDE_IEC61850_H_
