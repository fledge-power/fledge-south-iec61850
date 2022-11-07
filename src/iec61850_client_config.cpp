/*
 * Fledge IEC 61850 south plugin.
 *
 * Copyright (c) 2022, RTE (https://www.rte-france.com)
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Mikael Bourhis-Cloarec
 */

#include "./iec61850_client_config.h"

#include <arpa/inet.h>

// Fledge headers
#include <logger.h>

const char *const JSON_PROTOCOL_STACK = "protocol_stack";
const char *const JSON_TRANSPORT_LAYER = "transport_layer";
const char *const JSON_APPLICATION_LAYER = "application_layer";
const char *const JSON_EXCHANGED_DATA = "exchanged_data";

const char *const DEFAULT_LOG_MIN_LEVEL = "info";

const char *const DEFAULT_SERVER_NAME = "simpleIO";
const char *const DEFAULT_AP_NAME = "accessPoint0";
const char *const DEFAULT_IED_IP_ADDRESS = "127.0.0.1";
constexpr const uint16_t DEFAULT_MMS_PORT = 8102;

void IEC61850ClientConfig::importConfig(const ConfigCategory &newConfig)
{
    if (newConfig.itemExists("log min level")) {
        logMinLevel = newConfig.getValue("log min level");
    } else {
        logMinLevel = DEFAULT_LOG_MIN_LEVEL;
    }
    Logger::getLogger()->info("IEC61850ClientConfig: logMinLevel = %s",
                              logMinLevel.c_str());

    if (newConfig.itemExists("asset")) {
        assetName = newConfig.getValue("asset");
    } else {
        assetName = "iec61850";
    }
    Logger::getLogger()->info("IEC61850ClientConfig: assetName = %s",
                              assetName.c_str());

    importJsonProtocolConfig(newConfig.getValue(JSON_PROTOCOL_STACK));
    importJsonExchangeConfig(newConfig.getValue(JSON_EXCHANGED_DATA));
}

void IEC61850ClientConfig::importJsonProtocolConfig(const std::string &protocolConfig)
{
    rapidjson::Document document;

    /** Parse the input JSON std::string */
    if (document.Parse(protocolConfig.c_str()).HasParseError()) {
        Logger::getLogger()->fatal("Config: 'Protocol stack' parsing error");
        return;
    }

    /** Check if 'protocol_stack' is found */
    if (!document.IsObject()) {
        Logger::getLogger()->fatal("Config: 'Protocol stack' empty conf");
        return;
    }

    if (!document.HasMember(JSON_PROTOCOL_STACK) || !document[JSON_PROTOCOL_STACK].IsObject()) {
        Logger::getLogger()->fatal("Config: 'Protocol stack' empty conf");
        return;
    }

    const rapidjson::Value& protocolStack = document[JSON_PROTOCOL_STACK];

    /** Check if 'transport_layer' is found */
    if (!protocolStack.HasMember(JSON_TRANSPORT_LAYER) || !protocolStack[JSON_TRANSPORT_LAYER].IsObject()) {
        Logger::getLogger()->fatal("Config: 'transport layer' configuration is missing");
        return;
    }

    const rapidjson::Value &transportLayer = protocolStack[JSON_TRANSPORT_LAYER];

    importJsonTransportLayerConfig(transportLayer);

    /** Check if 'application_layer' is found */
    if (!protocolStack.HasMember(JSON_APPLICATION_LAYER) || !protocolStack[JSON_APPLICATION_LAYER].IsObject()) {
        Logger::getLogger()->fatal("Config: 'application layer' configuration is missing");
        return;
    }

    const rapidjson::Value &applicationLayer = protocolStack[JSON_APPLICATION_LAYER];

    importJsonApplicationLayerConfig(applicationLayer);
}

void IEC61850ClientConfig::importJsonTransportLayerConfig(const rapidjson::Value &transportLayer)
{
    // Preconditions
    if (! transportLayer.HasMember("connections")) {
        Logger::getLogger()->fatal("Config: 'Transport Layer' parsing error: no 'connections'");
        return;
    }
    if (! transportLayer["connections"].IsArray()) {
        Logger::getLogger()->fatal("Config: 'connections' is not an array -> fail to parse 'Transport Layer'");
        return;
    }

    const rapidjson::Value& connections = transportLayer["connections"];

    /** Parse each 'connection' JSON structure */
    for (auto &conn: connections.GetArray()) {
        importJsonConnectionConfig(conn);
    }
}

void IEC61850ClientConfig::importJsonConnectionConfig(const rapidjson::Value &connConfig)
{
    // Preconditions
    if (!connConfig.IsObject()) {
        Logger::getLogger()->fatal("Config: 'Connection' is not valid");
        return;
    }

    ServerConnectionParameters iedConnectionParam;

    /** Parse the simple JSON fields of 'connection' */
    if (connConfig.HasMember("server_name")) {
        if (connConfig["server_name"].IsString()) {
            iedConnectionParam.serverName = std::string(connConfig["server_name"].GetString());
        } else {
            iedConnectionParam.serverName = std::string(DEFAULT_SERVER_NAME);
        }
    }

    if (connConfig.HasMember("ap_name")) {
        if (connConfig["ap_name"].IsString()) {
            iedConnectionParam.apName = std::string(connConfig["ap_name"].GetString());
        } else {
            iedConnectionParam.apName = std::string(DEFAULT_AP_NAME);
        }
    }

    /** Parse the 'Address' JSON structure of 'connection' */
    iedConnectionParam.ipAddress = std::string(DEFAULT_IED_IP_ADDRESS);
    iedConnectionParam.mmsPort = DEFAULT_MMS_PORT;

    if (connConfig.HasMember("address")) {
        if (connConfig["address"].IsObject()) {
            const rapidjson::Value &addressStruct = connConfig["address"];

            importJsonServerAddressConfig(addressStruct,
                                          iedConnectionParam);

        }
    }

    logParsedIedConnectionParam(iedConnectionParam);

    ServerDictKey key = buildKey(iedConnectionParam);

    serverConfigDict[key] = iedConnectionParam;
}

void IEC61850ClientConfig::importJsonServerAddressConfig(const rapidjson::Value &serverAddressConfig,
                                                         ServerConnectionParameters &io_iedConnectionParam)
{
    // Preconditions
    if (!serverAddressConfig.IsObject()) {
        Logger::getLogger()->fatal("Config: 'Server address' is not valid");
        return;
    }

    if (serverAddressConfig.HasMember("ip_address")) {
        if (serverAddressConfig["ip_address"].IsString()) {
            io_iedConnectionParam.ipAddress = std::string(serverAddressConfig["ip_address"].GetString());
            if ( ! isValidIPAddress(io_iedConnectionParam.ipAddress)) {
                io_iedConnectionParam.ipAddress = std::string(DEFAULT_IED_IP_ADDRESS);
                Logger::getLogger()->warn("Config: IP address not valid; keep default IP address");
            }
        } else {
            Logger::getLogger()->warn("Config: wrong 'IP address' format; keep default IP address");
        }
    } else {
        Logger::getLogger()->warn("Config: 'IP address' is missing; keep default IP address");
    }

    if (serverAddressConfig.HasMember("mms_port")) {
        if (serverAddressConfig["mms_port"].IsInt()) {
            io_iedConnectionParam.mmsPort = serverAddressConfig["mms_port"].GetInt();
        } else {
            Logger::getLogger()->warn("Config: wrong 'MMS port' format; keep default MMS port");
        }
    } else {
        Logger::getLogger()->warn("Config: 'MMS port' is missing; keep default MMS port");
    }
}

void IEC61850ClientConfig::importJsonApplicationLayerConfig(const rapidjson::Value &/*applicationLayer*/) const
{}

void IEC61850ClientConfig::logParsedIedConnectionParam(const ServerConnectionParameters &iedConnectionParam)
{
    Logger::getLogger()->info("Config: Transport Layer: new IED:");
    Logger::getLogger()->info("Config: IED: server_name: %s", iedConnectionParam.serverName.c_str());
    Logger::getLogger()->info("Config: IED: ap_name:     %s", iedConnectionParam.apName.c_str());
    Logger::getLogger()->info("Config: IED: IP address:  %s", iedConnectionParam.ipAddress.c_str());
    Logger::getLogger()->info("Config: IED: MMS port:    %u", iedConnectionParam.mmsPort);
}


void IEC61850ClientConfig::importJsonExchangeConfig(const std::string &exchangeConfig)
{
    rapidjson::Document document;

    /** Parse the input JSON std::string */
    if (document.Parse(exchangeConfig.c_str()).HasParseError()) {
        Logger::getLogger()->fatal("Config: 'Exchanged data' parsing error");
        return;
    }

    /** The 'exchanged_data' section is mandatory */
    if (!document.IsObject()) {
        Logger::getLogger()->fatal("Config: 'Exchanged data' empty conf");
        return;
    }

    if (!document.HasMember(JSON_EXCHANGED_DATA) || !document[JSON_EXCHANGED_DATA].IsObject()) {
        Logger::getLogger()->fatal("Config: 'Exchanged data' empty conf");
        return;
    }

    const rapidjson::Value& exchange = document[JSON_EXCHANGED_DATA];

    /** The 'Logical Device' param is mandatory */
    if (exchange.HasMember("Logical Device")) {
        if (exchange["Logical Device"].IsString()) {
            exchangedData.logicalDeviceName = std::string(exchange["Logical Device"].GetString());
        } else {
            Logger::getLogger()->fatal("Config: 'Logical Device' wrong format");
        }
    } else {
        Logger::getLogger()->fatal("Config: 'Logical Device' is missing");
    }

    /** The 'Logical Node' param is mandatory */
    if (exchange.HasMember("Logical Node")) {
        if (exchange["Logical Node"].IsString()) {
            exchangedData.logicalNodeName = std::string(exchange["Logical Node"].GetString());
        } else {
            Logger::getLogger()->fatal("Config: 'Logical Node' wrong format");
        }
    } else {
        Logger::getLogger()->fatal("Config: 'Logical Node' is missing");
    }

    /** The 'CDC' param is mandatory */
    if (exchange.HasMember("CDC")) {
        if (exchange["CDC"].IsString()) {
            exchangedData.cdc = std::string(exchange["CDC"].GetString());
        } else {
            Logger::getLogger()->fatal("Config: 'CDC' wrong format");
        }
    } else {
        Logger::getLogger()->fatal("Config: 'CDC' is missing");
    }

    /** The 'Data Attribute' param is mandatory */
    if (exchange.HasMember("Data Attribute")) {
        if (exchange["Data Attribute"].IsString()) {
            exchangedData.dataAttribute = std::string(exchange["Data Attribute"].GetString());
        } else {
            Logger::getLogger()->fatal("Config: 'Data Attribute' wrong format");
        }
    } else {
        Logger::getLogger()->fatal("Config: 'Data Attribute' is missing");
    }

    /** The 'Functional Constraint' param is mandatory */
    if (exchange.HasMember("Functional Constraint")) {
        if (exchange["Functional Constraint"].IsString()) {
            exchangedData.fcName = std::string(exchange["Functional Constraint"].GetString());
        } else {
            Logger::getLogger()->fatal("Config: 'Functional Constraint' wrong format");
        }
    } else {
        Logger::getLogger()->fatal("Config: 'Functional Constraint' is missing");
    }

    exchangedData.daPathWithoutServerName = exchangedData.logicalDeviceName +
                                            "/" +
                                            exchangedData.logicalNodeName +
                                            "." +
                                            exchangedData.cdc +
                                            "." +
                                            exchangedData.dataAttribute;
}

bool IEC61850ClientConfig::isValidIPAddress(const std::string &addrStr)
{
    // see https://stackoverflow.com/questions/318236/how-do-you-validate-that-a-string-is-a-valid-ipv4-address-in-c
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, addrStr.c_str(), &(sa.sin_addr));

    return (result == 1);
}
