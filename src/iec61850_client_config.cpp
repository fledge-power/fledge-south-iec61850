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
const char *const DEFAULT_ASSET_NAME = "iec61850";

void IEC61850ClientConfig::importConfig(const ConfigCategory &newConfig)
{
    if (newConfig.itemExists("log min level")) {
        logMinLevel = newConfig.getValue("log min level");
    } else {
        logMinLevel = DEFAULT_LOG_MIN_LEVEL;
    }
    Logger::getLogger()->info("IEC61850ClientConfig: logMinLevel = %s",
                              logMinLevel.c_str());

    assetName = DEFAULT_ASSET_NAME;
    if (newConfig.itemExists("asset")) {
        assetName = newConfig.getValue("asset");
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
        throw ConfigurationException("'Protocol stack' parsing error");
    }

    /** The 'protocol_stack' section is mandatory */
    if (!document.IsObject()) {
        throw ConfigurationException("'Protocol stack' empty conf");
    }

    if (!document.HasMember(JSON_PROTOCOL_STACK) || !document[JSON_PROTOCOL_STACK].IsObject()) {
        throw ConfigurationException("'Protocol stack' empty conf");
    }

    const rapidjson::Value& protocolStack = document[JSON_PROTOCOL_STACK];

    /** Check if 'transport_layer' is found */
    if (!protocolStack.HasMember(JSON_TRANSPORT_LAYER) || !protocolStack[JSON_TRANSPORT_LAYER].IsObject()) {
        throw ConfigurationException("'transport layer' configuration is missing");
    }

    const rapidjson::Value &transportLayer = protocolStack[JSON_TRANSPORT_LAYER];

    importJsonTransportLayerConfig(transportLayer);

    /** Check if 'application_layer' is found */
    if (!protocolStack.HasMember(JSON_APPLICATION_LAYER) || !protocolStack[JSON_APPLICATION_LAYER].IsObject()) {
        throw ConfigurationException("'application layer' configuration is missing");
    }

    const rapidjson::Value &applicationLayer = protocolStack[JSON_APPLICATION_LAYER];

    importJsonApplicationLayerConfig(applicationLayer);
}

void IEC61850ClientConfig::importJsonTransportLayerConfig(const rapidjson::Value &transportLayer)
{
    // Preconditions
    if (! transportLayer.HasMember("ied_name")) {
        throw ConfigurationException("the mandatory 'ied_name' not found");
    }
    if (! transportLayer["ied_name"].IsString()) {
        throw ConfigurationException("bad format for the mandatory 'ied_name'");
    }
    if (! transportLayer.HasMember("connections")) {
        throw ConfigurationException("'Transport Layer' parsing error: no 'connections'");
    }
    if (! transportLayer["connections"].IsArray()) {
        throw ConfigurationException("'connections' is not an array -> fail to parse 'Transport Layer'");
    }

    iedName = std::string(transportLayer["ied_name"].GetString());

    const rapidjson::Value& connections = transportLayer["connections"];

    /** Parse each 'connection' JSON structure */
    for (auto &conn: connections.GetArray()) {
        importJsonConnectionConfig(conn);
    }
}

void IEC61850ClientConfig::importJsonConnectionConfig(const rapidjson::Value &connConfig)
{
    // Preconditions
    if (! connConfig.IsObject()) {
        throw ConfigurationException("'Connection' is not valid");
    }
    if (! connConfig.HasMember("srv_ip")) {
        throw ConfigurationException("the mandatory 'srv_ip' not found");
    }
    if (! connConfig["srv_ip"].IsString()) {
        throw ConfigurationException("bad format for the mandatory 'srv_ip'");
    }
    if (! connConfig.HasMember("port")) {
        throw ConfigurationException("the mandatory 'port' not found");
    }
    if (! connConfig["port"].IsInt()) {
        throw ConfigurationException("bad format for the mandatory 'port'");
    }

    ServerConnectionParameters iedConnectionParam;

    iedConnectionParam.ipAddress = std::string(connConfig["srv_ip"].GetString());

    if ( ! isValidIPAddress(iedConnectionParam.ipAddress)) {
        throw ConfigurationException("not a valid IP address for the mandatory 'srv_ip'");
    }

    iedConnectionParam.mmsPort = connConfig["port"].GetInt();

    logParsedIedConnectionParam(iedConnectionParam);

    ServerDictKey key = buildKey(iedConnectionParam);

    serverConfigDict[key] = iedConnectionParam;
}

void IEC61850ClientConfig::importJsonApplicationLayerConfig(const rapidjson::Value &/*applicationLayer*/) const
{}

void IEC61850ClientConfig::logParsedIedConnectionParam(const ServerConnectionParameters &iedConnectionParam)
{
    Logger::getLogger()->info("Config: Transport Layer: new IED:");
    Logger::getLogger()->info("Config: IED: ied_name: %s", iedName.c_str());
    Logger::getLogger()->info("Config: IED: IP address:  %s", iedConnectionParam.ipAddress.c_str());
    Logger::getLogger()->info("Config: IED: MMS port:    %u", iedConnectionParam.mmsPort);
}


void IEC61850ClientConfig::importJsonExchangeConfig(const std::string &exchangeConfig)
{
    rapidjson::Document document;

    /** Parse the input JSON std::string */
    if (document.Parse(exchangeConfig.c_str()).HasParseError()) {
        throw ConfigurationException("'Exchanged data' parsing error");
    }

    /** The 'exchanged_data' section is mandatory */
    if (!document.IsObject()) {
        throw ConfigurationException("'Exchanged data' empty conf");
    }

    if (!document.HasMember(JSON_EXCHANGED_DATA) || !document[JSON_EXCHANGED_DATA].IsObject()) {
        throw ConfigurationException("'Exchanged data' empty conf");
    }

    const rapidjson::Value& exchange = document[JSON_EXCHANGED_DATA];

    /** The 'Logical Device' param is mandatory */
    if (! exchange.HasMember("Logical Device")) {
        throw ConfigurationException("the mandatory 'Logical Device' not found");
    }
    if (! exchange["Logical Device"].IsString()) {
        throw ConfigurationException("bad format for the mandatory 'Logical Device'");
    }

    exchangedData.logicalDeviceName = std::string(exchange["Logical Device"].GetString());

    /** The 'Logical Node' param is mandatory */
    if (! exchange.HasMember("Logical Node")) {
        throw ConfigurationException("the mandatory 'Logical Node' not found");
    }
    if (! exchange["Logical Node"].IsString()) {
        throw ConfigurationException("bad format for the mandatory 'Logical Node'");
    }

    exchangedData.logicalNodeName = std::string(exchange["Logical Node"].GetString());

    /** The 'CDC' param is mandatory */
    if (! exchange.HasMember("CDC")) {
        throw ConfigurationException("the mandatory 'CDC' not found");
    }
    if (! exchange["CDC"].IsString()) {
        throw ConfigurationException("bad format for the mandatory 'CDC'");
    }

    exchangedData.cdc = std::string(exchange["CDC"].GetString());

    /** The 'Data Attribute' param is mandatory */
    if (! exchange.HasMember("Data Attribute")) {
        throw ConfigurationException("the mandatory 'Data Attribute' not found");
    }
    if (! exchange["Data Attribute"].IsString()) {
        throw ConfigurationException("bad format for the mandatory 'Data Attribute'");
    }

    exchangedData.dataAttribute = std::string(exchange["Data Attribute"].GetString());

    /** The 'Functional Constraint' param is mandatory */
    if (! exchange.HasMember("Functional Constraint")) {
        throw ConfigurationException("the mandatory 'Functional Constraint' not found");
    }
    if (! exchange["Functional Constraint"].IsString()) {
        throw ConfigurationException("bad format for the mandatory 'Functional Constraint'");
    }

    exchangedData.fcName = std::string(exchange["Functional Constraint"].GetString());

    exchangedData.daPath = iedName + exchangedData.logicalDeviceName +
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
