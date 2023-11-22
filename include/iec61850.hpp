#ifndef INCLUDE_IEC61850_H_
#define INCLUDE_IEC61850_H_

#include <cstdint>
#include <logger.h>
#include <plugin_api.h>
#include <reading.h>
#include <gtest/gtest.h>

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
#include <unordered_map>

#include "iec61850_client_config.hpp"
#include "iec61850_client_connection.hpp"

#define BACKUP_CONNECTION_TIMEOUT 5000

class IEC61850Client;


class PivotTimestamp
{
public:
    explicit PivotTimestamp(Datapoint* timestampData);
    explicit PivotTimestamp(uint64_t ms);
    ~PivotTimestamp() = default;

    void setTimeInMs(uint64_t ms);

    int SecondSinceEpoch() const;
    int FractionOfSecond() const;
    uint64_t getTimeInMs() const;

    bool ClockFailure() const {return m_clockFailure;};
    bool LeapSecondKnown() const {return m_leapSecondKnown;};
    bool ClockNotSynchronized() const {return m_clockNotSynchronized;};
    int TimeAccuracy() const {return m_timeAccuracy;};

    static uint64_t GetCurrentTimeInMs();

private:

    void handleTimeQuality(Datapoint* timeQuality);

    std::vector<uint8_t> m_valueArray = std::vector<uint8_t>(7);

    int m_secondSinceEpoch;
    int m_fractionOfSecond;

    int m_timeAccuracy;
    bool m_clockFailure = false;
    bool m_leapSecondKnown = false;
    bool m_clockNotSynchronized = false;
};


class IEC61850
{
public:
    using INGEST_CB = void (*)(void*, Reading);

    IEC61850() = default;
    ~IEC61850();

    void setAssetName(const std::string& asset) { m_asset = asset; }
    void setJsonConfig(const std::string& stack_configuration,
                              const std::string& msg_configuration,
                              const std::string& tls_configuration);

    void start();
    void stop();

    void ingest(const std::string& assetName, const std::vector<Datapoint*>& points);
    void registerIngest(void* data, void (*cb)(void*, Reading));
    bool operation(const std::string& operation, int count,
                   PLUGIN_PARAMETER** params);


private:

    IEC61850ClientConfig* m_config = new IEC61850ClientConfig();;

    std::string m_asset;


    INGEST_CB m_ingest = nullptr;  // Callback function used to send data to south service
    void* m_data;        // Ingest function data
    IEC61850Client* m_client = nullptr;
    FRIEND_TEST(ConnectionHandlingTest,   SingleConnection);
    FRIEND_TEST(ConnectionHandlingTest,   SingleConnectionReconnect);
    FRIEND_TEST(ControlTest,   SingleCommandDirectNormal);
    FRIEND_TEST(ControlTest,   SingleCommandDirectEnhanced);
    FRIEND_TEST(ControlTest,   SingleCommandSetValue);
    FRIEND_TEST(ReportingTest,   ReportingWithStaticDataset);
    FRIEND_TEST(ReportingTest,   ReportingWithDynamicDataset);
    FRIEND_TEST(ReportingTest,   ReportingGI);
    FRIEND_TEST(ReportingTest,   ReportingSetpointCommand);
    FRIEND_TEST(SpontDataTest,   Polling);
};


class IEC61850Client
{
public:

    explicit IEC61850Client(IEC61850* iec61850, IEC61850ClientConfig* config);

    ~IEC61850Client();

    void sendData(const std::vector<Datapoint*>& data,
                  const std::vector<std::string>& labels);

    void start();

    void stop();
    
    void prepareConnections();
    
    void handleValue(std::string objRef, MmsValue* mmsValue);
    void handleAllValues();

    bool handleOperation(Datapoint* operation);

    void logIedClientError(IedClientError err, const std::string& info) const;

    void sendCommandAck(const std::string& label, ControlModel mode, bool terminated);


private:
    std::shared_ptr<std::vector<IEC61850ClientConnection*>> m_connections = nullptr;
    
    IEC61850ClientConnection* m_active_connection = nullptr;
    std::mutex m_activeConnectionMtx;
    
    enum class ConnectionStatus
    {
        STARTED,
        NOT_CONNECTED
    };

    ConnectionStatus m_connStatus = ConnectionStatus::NOT_CONNECTED;

    void updateConnectionStatus(ConnectionStatus newState);

    std::mutex connectionsMutex;
    std::mutex statusMutex;

    std::thread* m_monitoringThread = nullptr;
    void _monitoringThread();

    bool m_started = false;

    IEC61850ClientConfig* m_config;
    IEC61850* m_iec61850;

    template <class T> Datapoint* m_createDatapoint(const std::string& label, const std::string& objRef, T value, Quality quality, uint64_t timestampMs);
    static int getRootFromCDC(const CDCTYPE cdc);
   
    void addQualityDp(Datapoint* cdcDp, Quality quality) const;
    void addTimestampDp(Datapoint* cdcDp, uint64_t timestampMs) const;
    template <class T> void addValueDp(Datapoint* cdcDp, CDCTYPE type, T value) const;

    void m_handleMonitoringData(const std::string& objRef, std::vector<Datapoint*>& datapoints, const std::string& label, CDCTYPE type, MmsValue* mmsValue, const std::string& variable, FunctionalConstraint fc);
    std::unordered_map<std::string, Datapoint*> m_outstandingCommands;
    FRIEND_TEST(ConnectionHandlingTest,   SingleConnection);
    FRIEND_TEST(ConnectionHandlingTest,   SingleConnectionReconnect);
    FRIEND_TEST(ControlTest,   SingleCommandDirectNormal);
    FRIEND_TEST(ControlTest,   SingleCommandDirectEnhanced);
    FRIEND_TEST(ControlTest,   SingleCommandSetValue);
    FRIEND_TEST(ReportingTest,   ReportingWithStaticDataset);
    FRIEND_TEST(ReportingTest,   ReportingWithDynamicDataset);
    FRIEND_TEST(ReportingTest,   ReportingGI);
    FRIEND_TEST(ReportingTest,   ReportingSetpointCommand);
    FRIEND_TEST(SpontDataTest,   Polling);
};
#endif  // INCLUDE_IEC61850_H_
