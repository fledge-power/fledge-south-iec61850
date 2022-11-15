#ifndef INCLUDE_IEC61850_CLIENT_CONFIG_H_
#define INCLUDE_IEC61850_CLIENT_CONFIG_H_

/*
 * Fledge IEC 61850 south plugin.
 *
 * Copyright (c) 2022, RTE (https://www.rte-france.com)
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Mikael Bourhis-Cloarec
 */

#include <string>
#include <map>

// Fledge headers
#include <config_category.h>

#include <rapidjson/document.h>

struct ServerConnectionParameters {
    std::string ipAddress;
    uint16_t    mmsPort;
};

struct ExchangedData {
    std::string logicalDeviceName = "LD_NOT_DEFINED";
    std::string logicalNodeName = "LN_NOT_DEFINED";
    std::string cdc = "CDC_NOT_DEFINED";
    std::string dataAttribute = "DA_NOT_DEFINED";
    std::string fcName = "FC_NOT_DEFINED";

    std::string daPath = "NOT_DEFINED";
};

using ServerDictKey = std::string;
using ServerConfigDict = std::map<ServerDictKey, ServerConnectionParameters, std::less<>>;



class IEC61850ClientConfig
{
    public :

        std::string logMinLevel;
        std::string assetName;
        std::string iedName;

        ServerConfigDict serverConfigDict;

        // Data model section
        ExchangedData exchangedData;

        void importConfig(const ConfigCategory &newConfig);

        inline static std::string buildKey(const ServerConnectionParameters &serverConn) {
            return (serverConn.ipAddress + "_" +
                    std::to_string(serverConn.mmsPort));
        }

    private:
        void importJsonProtocolConfig(const std::string &protocolConfig);
        void importJsonTransportLayerConfig(const rapidjson::Value &transportLayer);
        void importJsonConnectionConfig(const rapidjson::Value &connConfig);
        void importJsonApplicationLayerConfig(const rapidjson::Value &transportLayer) const;
        void importJsonExchangeConfig(const std::string &exchangeConfig);

        void logParsedIedConnectionParam(const ServerConnectionParameters &iedConnectionParam);
        static bool isValidIPAddress(const std::string &addrStr);
};

#endif  // INCLUDE_IEC61850_CLIENT_CONFIG_H_
