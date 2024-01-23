#include <algorithm>
#include <arpa/inet.h>
#include <iec61850.hpp>
#include <iec61850_client_config.hpp>
#include <regex>
#include <vector>

#define JSON_PROTOCOL_STACK "protocol_stack"
#define JSON_TRANSPORT_LAYER "transport_layer"
#define JSON_APPLICATION_LAYER "application_layer"
#define JSON_DATASETS "datasets"
#define JSON_CONNECTIONS "connections"
#define JSON_IP "ip_addr"
#define JSON_PORT "port"
#define JSON_TLS "tls"
#define JSON_DATASET_REF "dataset_ref"
#define JSON_DATASET_ENTRIES "entries"
#define JSON_POLLING_INTERVAL "polling_interval"
#define JSON_REPORT_SUBSCRIPTIONS "report_subscriptions"
#define JSON_RCB_REF "rcb_ref"
#define JSON_TRGOPS "trgops"

#define JSON_EXCHANGED_DATA "exchanged_data"
#define JSON_DATAPOINTS "datapoints"
#define JSON_PROTOCOLS "protocols"
#define JSON_LABEL "label"
#define JSON_PIVOT_ID "pivot_id"

#define PROTOCOL_IEC61850 "iec61850"
#define JSON_PROT_NAME "name"
#define JSON_PROT_OBJ_REF "objref"
#define JSON_PROT_CDC "cdc"

using namespace rapidjson;

static const std::unordered_map<std::string, int> trgOptions
    = { { "data_changed", TRG_OPT_DATA_CHANGED },
        { "quality_changed", TRG_OPT_QUALITY_CHANGED },
        { "data_update", TRG_OPT_DATA_UPDATE },
        { "integrity", TRG_OPT_INTEGRITY },
        { "gi", TRG_OPT_GI },
        { "transient", TRG_OPT_TRANSIENT } };

static const std::unordered_map<std::string, CDCTYPE> cdcMap
    = { { "SpsTyp", SPS }, { "DpsTyp", DPS }, { "BscTyp", BSC },
        { "MvTyp", MV },   { "SpcTyp", SPC }, { "DpcTyp", DPC },
        { "ApcTyp", APC }, { "IncTyp", INC }, { "InsTyp", INS },
        { "SpgTyp", SPG }, { "EnsTyp", ENS }, { "AsgTyp", ASG },
        { "IngTyp", ING } };

int
IEC61850ClientConfig::getCdcTypeFromString (const std::string& cdc)
{
    auto it = cdcMap.find (cdc);
    if (it != cdcMap.end ())
    {
        return it->second;
    }
    return -1; // LCOV_EXCL_LINE
}

std::shared_ptr<DataExchangeDefinition>
IEC61850ClientConfig::getExchangeDefinitionByLabel (const std::string& label)
{
    auto it = m_exchangeDefinitions.find (label);
    if (it != m_exchangeDefinitions.end ())
    {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<DataExchangeDefinition>
IEC61850ClientConfig::getExchangeDefinitionByPivotId (
    const std::string& pivotId)
{
    auto it = m_exchangeDefinitionsPivotId.find (pivotId);
    if (it != m_exchangeDefinitionsPivotId.end ())
    {
        return it->second;
    }
    return nullptr;
}

bool
IEC61850ClientConfig::isValidIPAddress (const std::string& addrStr)
{
    // see
    // https://stackoverflow.com/questions/318236/how-do-you-validate-that-a-string-is-a-valid-ipv4-address-in-c
    struct sockaddr_in sa;
    int result = inet_pton (AF_INET, addrStr.c_str (),
                            &(sa.sin_addr)); // LCOV_EXCL_LINE

    return (result == 1);
}

void
IEC61850ClientConfig::deleteExchangeDefinitions ()
{
    m_exchangeDefinitions.clear ();
    m_exchangeDefinitionsObjRef.clear ();
    m_exchangeDefinitionsPivotId.clear ();
    m_polledDatapoints.clear ();
}

IEC61850ClientConfig::~IEC61850ClientConfig ()
{
    deleteExchangeDefinitions ();
}

void
IEC61850ClientConfig::importProtocolConfig (const std::string& protocolConfig)
{
    m_protocolConfigComplete = false;

    Document document;

    if (document.Parse (protocolConfig.c_str ()).HasParseError ())
    {
        Iec61850Utility::log_fatal ("Parsing error in protocol configuration");
        return;
    }

    if (!document.IsObject ())
    {
        return; // LCOV_EXCL_LINE
    }

    if (!document.HasMember (JSON_PROTOCOL_STACK)
        || !document[JSON_PROTOCOL_STACK].IsObject ())
    {
        return;
    }

    const Value& protocolStack = document[JSON_PROTOCOL_STACK];

    if (!protocolStack.HasMember (JSON_TRANSPORT_LAYER)
        || !protocolStack[JSON_TRANSPORT_LAYER].IsObject ())
    {
        Iec61850Utility::log_fatal (
            "transport layer configuration is missing");
        return;
    }

    const Value& transportLayer = protocolStack[JSON_TRANSPORT_LAYER];

    if (!transportLayer.HasMember (JSON_CONNECTIONS)
        || !transportLayer[JSON_CONNECTIONS].IsArray ())
    {
        Iec61850Utility::log_fatal ("no connections are configured");
        return;
    }

    const Value& connections = transportLayer[JSON_CONNECTIONS];

    for (const Value& connection : connections.GetArray ())
    {
        if (connection.HasMember (JSON_IP) && connection[JSON_IP].IsString ())
        {
            std::string srvIp = connection[JSON_IP].GetString ();
            if (!isValidIPAddress (srvIp))
            {
                Iec61850Utility::log_error ("Invalid Ip address %s",
                                            srvIp.c_str ());
                continue;
            }
            if (connection.HasMember (JSON_PORT)
                && connection[JSON_PORT].IsInt ())
            {
                int portVal = connection[JSON_PORT].GetInt ();
                if (portVal <= 0 || portVal >= 65636)
                {
                    Iec61850Utility::log_error ("Invalid port %d", portVal);
                    continue;
                }
            }

            auto group = std::make_shared<RedGroup> ();

            group->ipAddr = connection[JSON_IP].GetString ();
            group->tcpPort = connection[JSON_PORT].GetInt ();
            group->tls = false;

            if (connection.HasMember ("osi"))
            {
                importJsonConnectionOsiConfig (connection["osi"], *group);
            }

            if (connection.HasMember ("tls"))
            {
                if (connection["tls"].IsBool ())
                {
                    group->tls = (connection["tls"].GetBool ());
                }
                else
                {
                    printf (
                        "connection.tls has invalid type -> not using TLS\n");
                    Iec61850Utility::log_warn (
                        "connection.tls has invalid type -> not using TLS");
                }
            }

            m_connections.push_back (group);
        }
    }

    if (transportLayer.HasMember ("backupTimeout")
        && transportLayer["backupTimeout"].IsInt ())
    {
        m_backupConnectionTimeout = transportLayer["backupTimeout"].GetInt ();
    }

    if (!protocolStack.HasMember (JSON_APPLICATION_LAYER)
        || !protocolStack[JSON_APPLICATION_LAYER].IsObject ())
    {
        Iec61850Utility::log_fatal (
            "transport layer configuration is missing");
        return;
    }

    const Value& applicationLayer = protocolStack[JSON_APPLICATION_LAYER];

    if (applicationLayer.HasMember (JSON_POLLING_INTERVAL))
    {
        if (!applicationLayer[JSON_POLLING_INTERVAL].IsInt ())
        {
            Iec61850Utility::log_error (
                "polling_interval has invalid data type");
            return;
        }
        int intVal = applicationLayer[JSON_POLLING_INTERVAL].GetInt ();
        if (intVal < 0)
        {
            Iec61850Utility::log_error ("polling_interval must be positive");
            return;
        }
        pollingInterval = intVal;
    }

    if (applicationLayer.HasMember (JSON_DATASETS)
        && applicationLayer[JSON_DATASETS].IsArray ())
    {
        for (const auto& datasetVal :
             applicationLayer[JSON_DATASETS].GetArray ())
        {
            if (!datasetVal.IsObject ()
                || !datasetVal.HasMember (JSON_DATASET_REF)
                || !datasetVal[JSON_DATASET_REF].IsString ())
                continue;

            std::string datasetRef = datasetVal[JSON_DATASET_REF].GetString ();
            auto dataset = std::make_shared<Dataset> ();
            dataset->datasetRef = datasetRef;
            if (datasetVal.HasMember (JSON_DATASET_ENTRIES)
                && datasetVal[JSON_DATASET_ENTRIES].IsArray ())
            {
                for (const auto& entryVal :
                     datasetVal[JSON_DATASET_ENTRIES].GetArray ())
                {
                    if (entryVal.IsString ())
                    {
                        std::string objref = entryVal.GetString ();
                        Iec61850Utility::log_debug (
                            "Add entry %s to dataset %s", objref.c_str (),
                            datasetRef.c_str ());
                        dataset->entries.push_back (objref);

                        std::string extractedObjRef = objref;
                        size_t secondDotPos = extractedObjRef.find (
                            '.', extractedObjRef.find ('.') + 1);
                        size_t bracketPos = extractedObjRef.find ('[');

                        if (secondDotPos != std::string::npos)
                        {
                            extractedObjRef.erase (secondDotPos);
                        }
                        else
                        {
                            extractedObjRef.erase (bracketPos);
                        }

                        const std::shared_ptr<DataExchangeDefinition> def
                            = getExchangeDefinitionByObjRef (extractedObjRef);

                        if (def)
                            m_polledDatapoints.erase (extractedObjRef);
                    }
                }
            }
            if (datasetVal.HasMember ("dynamic")
                && datasetVal["dynamic"].IsBool ())
            {
                dataset->dynamic = datasetVal["dynamic"].GetBool ();
            }
            else
            {
                Iec61850Utility::log_warn (
                    "Dataset %s has no dynamic value -> defaulting to static",
                    dataset->datasetRef.c_str ());
                dataset->dynamic = false;
            }
            m_datasets.insert ({ datasetRef, dataset });
        }
    }

    if (applicationLayer.HasMember (JSON_REPORT_SUBSCRIPTIONS)
        && applicationLayer[JSON_REPORT_SUBSCRIPTIONS].IsArray ())
    {
        for (const auto& reportVal :
             applicationLayer[JSON_REPORT_SUBSCRIPTIONS].GetArray ())
        {
            if (!reportVal.IsObject ())
                continue;
            auto report = std::make_shared<ReportSubscription> ();

            if (reportVal.HasMember (JSON_RCB_REF)
                && reportVal[JSON_RCB_REF].IsString ())
            {
                report->rcbRef = reportVal[JSON_RCB_REF].GetString ();
            }
            else
            {
                continue;
            }

            if (reportVal.HasMember (JSON_DATASET_REF)
                && reportVal[JSON_DATASET_REF].IsString ())
            {
                report->datasetRef = reportVal[JSON_DATASET_REF].GetString ();
            }
            else
            {
                report->datasetRef = "";
            }

            if (reportVal.HasMember (JSON_TRGOPS)
                && reportVal[JSON_TRGOPS].IsArray ())
            {
                for (const auto& trgopVal : reportVal[JSON_TRGOPS].GetArray ())
                {
                    if (trgopVal.IsString ())
                    {
                        auto it = trgOptions.find (trgopVal.GetString ());
                        if (it == trgOptions.end ())
                            continue; // LCOV_EXCL_LINE
                        report->trgops |= it->second;
                    }
                }
            }
            else
            {
                report->trgops = -1;
            }

            if (reportVal.HasMember ("buftm") && reportVal["buftm"].IsInt ())
            {
                report->buftm = reportVal["buftm"].GetInt ();
            }
            else
            {
                report->buftm = -1;
            }

            if (reportVal.HasMember ("intgpd") && reportVal["intgpd"].IsInt ())
            {
                report->intgpd = reportVal["intgpd"].GetInt ();
            }
            else
            {
                report->intgpd = -1;
            }

            if (reportVal.HasMember ("gi") && reportVal["gi"].IsBool ())
            {
                report->gi = reportVal["gi"].GetBool ();
            }
            else
            {
                Iec61850Utility::log_error (
                    "Report %s has no gi value, defaulting to disabled",
                    report->rcbRef.c_str ());
                report->gi = false;
            }

            m_reportSubscriptions.insert (
                { report->rcbRef, std::move (report) });
        }
    }

    m_protocolConfigComplete = true;
}

void
IEC61850ClientConfig::importJsonConnectionOsiConfig (
    const rapidjson::Value& connOsiConfig, RedGroup& iedConnectionParam)
{
    // Preconditions
    if (!connOsiConfig.IsObject ())
    {
        throw ConfigurationException ("'OSI' section is not valid");
    }

    OsiParameters* osiParams = &iedConnectionParam.osiParameters;

    // AE qualifiers
    if (connOsiConfig.HasMember ("local_ae_qualifier"))
    {
        if (!connOsiConfig["local_ae_qualifier"].IsInt ())
        {
            throw ConfigurationException (
                "bad format for 'local_ae_qualifier'");
        }

        osiParams->localAeQualifier
            = connOsiConfig["local_ae_qualifier"].GetInt ();
    }

    if (connOsiConfig.HasMember ("remote_ae_qualifier"))
    {
        if (!connOsiConfig["remote_ae_qualifier"].IsInt ())
        {
            throw ConfigurationException (
                "bad format for 'remote_ae_qualifier'");
        }

        osiParams->remoteAeQualifier
            = connOsiConfig["remote_ae_qualifier"].GetInt ();
    }

    // AP Title
    if (connOsiConfig.HasMember ("local_ap_title"))
    {
        if (!connOsiConfig["local_ap_title"].IsString ())
        {
            throw ConfigurationException ("bad format for 'local_ap_title'");
        }

        osiParams->localApTitle = connOsiConfig["local_ap_title"].GetString ();
        std::replace (osiParams->localApTitle.begin (),
                      osiParams->localApTitle.end (), ',', '.');

        // check 'localApTitle' contains digits and dot only
        std::string strToCheck = osiParams->localApTitle;
        strToCheck.erase (
            std::remove (strToCheck.begin (), strToCheck.end (), '.'),
            strToCheck.end ());

        if (!std::regex_match (strToCheck, std::regex ("[0-9]*")))
        {
            throw ConfigurationException ("'local_ap_title' is not valid");
        }
    }

    if (connOsiConfig.HasMember ("remote_ap_title"))
    {
        if (!connOsiConfig["remote_ap_title"].IsString ())
        {
            throw ConfigurationException ("bad format for 'remote_ap_title'");
        }

        osiParams->remoteApTitle
            = connOsiConfig["remote_ap_title"].GetString ();
        std::replace (osiParams->remoteApTitle.begin (),
                      osiParams->remoteApTitle.end (), ',', '.');
        // check 'remoteApTitle' contains digits and dot only
        std::string strToCheck = osiParams->remoteApTitle;
        strToCheck.erase (
            std::remove (strToCheck.begin (), strToCheck.end (), '.'),
            strToCheck.end ());

        if (!std::regex_match (strToCheck, std::regex ("[0-9]*")))
        {
            throw ConfigurationException ("'remote_ap_title' is not valid");
        }
    }

    // Selector
    importJsonConnectionOsiSelectors (connOsiConfig, osiParams);
    iedConnectionParam.isOsiParametersEnabled = true;
}

void
IEC61850ClientConfig::importJsonConnectionOsiSelectors (
    const rapidjson::Value& connOsiConfig, OsiParameters* osiParams)
{
    if (connOsiConfig.HasMember ("local_psel"))
    {
        if (!connOsiConfig["local_psel"].IsString ())
        {
            throw ConfigurationException ("bad format for 'local_psel'");
        }

        std::string inputOsiSelector
            = connOsiConfig["local_psel"].GetString ();
        osiParams->localPSelector.size
            = parseOsiPSelector (inputOsiSelector, &osiParams->localPSelector);
    }

    if (connOsiConfig.HasMember ("local_ssel"))
    {
        if (!connOsiConfig["local_ssel"].IsString ())
        {
            throw ConfigurationException ("bad format for 'local_ssel'");
        }

        std::string inputOsiSelector
            = connOsiConfig["local_ssel"].GetString ();
        osiParams->localSSelector.size
            = parseOsiSSelector (inputOsiSelector, &osiParams->localSSelector);
    }

    if (connOsiConfig.HasMember ("local_tsel"))
    {
        if (!connOsiConfig["local_tsel"].IsString ())
        {
            throw ConfigurationException ("bad format for 'local_tsel'");
        }

        std::string inputOsiSelector
            = connOsiConfig["local_tsel"].GetString ();
        osiParams->localTSelector.size
            = parseOsiTSelector (inputOsiSelector, &osiParams->localTSelector);
    }

    if (connOsiConfig.HasMember ("remote_psel"))
    {
        if (!connOsiConfig["remote_psel"].IsString ())
        {
            throw ConfigurationException ("bad format for 'remote_psel'");
        }

        std::string inputOsiSelector
            = connOsiConfig["remote_psel"].GetString ();
        osiParams->remotePSelector.size = parseOsiPSelector (
            inputOsiSelector, &osiParams->remotePSelector);
    }

    if (connOsiConfig.HasMember ("remote_ssel"))
    {
        if (!connOsiConfig["remote_ssel"].IsString ())
        {
            throw ConfigurationException ("bad format for 'remote_ssel'");
        }

        std::string inputOsiSelector
            = connOsiConfig["remote_ssel"].GetString ();
        osiParams->remoteSSelector.size = parseOsiSSelector (
            inputOsiSelector, &osiParams->remoteSSelector);
    }

    if (connOsiConfig.HasMember ("remote_tsel"))
    {
        if (!connOsiConfig["remote_tsel"].IsString ())
        {
            throw ConfigurationException ("bad format for 'remote_tsel'");
        }

        std::string inputOsiSelector
            = connOsiConfig["remote_tsel"].GetString ();
        osiParams->remoteTSelector.size = parseOsiTSelector (
            inputOsiSelector, &osiParams->remoteTSelector);
    }
}

OsiSelectorSize
IEC61850ClientConfig::parseOsiPSelector (std::string& inputOsiSelector,
                                         PSelector* pselector)
{
    return parseOsiSelector (inputOsiSelector, pselector->value, 16);
}

OsiSelectorSize
IEC61850ClientConfig::parseOsiSSelector (std::string& inputOsiSelector,
                                         SSelector* sselector)
{
    return parseOsiSelector (inputOsiSelector, sselector->value, 16);
}

OsiSelectorSize
IEC61850ClientConfig::parseOsiTSelector (std::string& inputOsiSelector,
                                         TSelector* tselector)
{
    return parseOsiSelector (inputOsiSelector, tselector->value, 4);
}

OsiSelectorSize
IEC61850ClientConfig::parseOsiSelector (std::string& inputOsiSelector,
                                        uint8_t* selectorValue,
                                        const uint8_t selectorSize)
{
    char* tokenContext = nullptr;
    const char* nextToken
        = strtok_r (&inputOsiSelector[0], " ,.-", &tokenContext);
    uint8_t count = 0;

    while (nullptr != nextToken)
    {
        if (count >= selectorSize)
        {
            throw ConfigurationException (
                "bad format for 'OSI Selector' (too many bytes)");
        }

        int base = 10;

        if (0 == strncmp (nextToken, "0x", 2))
        {
            base = 16;
        }

        unsigned long ul = 0;

        try
        {
            ul = std::stoul (nextToken, nullptr, base);
        }
        catch (std::invalid_argument&)
        {
            throw ConfigurationException (
                "bad format for 'OSI Selector' (not a byte)");
        }
        catch (std::out_of_range&)
        {
            throw ConfigurationException (
                "bad format for 'OSI Selector (exceed an int)'");
        }

        if (ul > 255)
        {
            throw ConfigurationException (
                "bad format for 'OSI Selector' (exceed a byte)");
        }

        selectorValue[count] = static_cast<uint8_t> (ul);
        count++;
        nextToken = strtok_r (nullptr, " ,.-", &tokenContext);
    }

    return count;
}

void
IEC61850ClientConfig::importExchangeConfig (const std::string& exchangeConfig)
{
    m_exchangeConfigComplete = false;

    deleteExchangeDefinitions ();

    Document document;

    if (document.Parse (exchangeConfig.c_str ()).HasParseError ())
    {
        Iec61850Utility::log_fatal (
            "Parsing error in data exchange configuration");

        return;
    }

    if (!document.IsObject ())
    {
        Iec61850Utility::log_error ("NO DOCUMENT OBJECT FOR EXCHANGED DATA");
        return;
    }
    if (!document.HasMember (JSON_EXCHANGED_DATA)
        || !document[JSON_EXCHANGED_DATA].IsObject ())
    {
        Iec61850Utility::log_error ("EXCHANGED DATA NOT AN OBJECT");
        return;
    }

    const Value& exchangeData = document[JSON_EXCHANGED_DATA];

    if (!exchangeData.HasMember (JSON_DATAPOINTS)
        || !exchangeData[JSON_DATAPOINTS].IsArray ())
    {
        Iec61850Utility::log_error ("NO EXCHANGED DATA DATAPOINTS");
        return;
    }

    const Value& datapoints = exchangeData[JSON_DATAPOINTS];

    for (const Value& datapoint : datapoints.GetArray ())
    {
        if (!datapoint.IsObject ())
        {
            Iec61850Utility::log_error ("DATAPOINT NOT AN OBJECT");
            return;
        }

        if (!datapoint.HasMember (JSON_LABEL)
            || !datapoint[JSON_LABEL].IsString ())
        {
            Iec61850Utility::log_error ("DATAPOINT MISSING LABEL");
            return;
        }
        std::string label = datapoint[JSON_LABEL].GetString ();

        if (!datapoint.HasMember (JSON_PIVOT_ID)
            || !datapoint[JSON_PIVOT_ID].IsString ())
        {
            Iec61850Utility::log_error ("DATAPOINT MISSING PIVOT ID");
            return;
        }

        std::string pivot_id = datapoint[JSON_PIVOT_ID].GetString ();

        if (!datapoint.HasMember (JSON_PROTOCOLS)
            || !datapoint[JSON_PROTOCOLS].IsArray ())
        {
            Iec61850Utility::log_error ("DATAPOINT MISSING PROTOCOLS ARRAY");
            return;
        }
        for (const Value& protocol : datapoint[JSON_PROTOCOLS].GetArray ())
        {
            if (!protocol.HasMember (JSON_PROT_NAME)
                || !protocol[JSON_PROT_NAME].IsString ())
            {
                Iec61850Utility::log_error ("PROTOCOL MISSING NAME");
                return;
            }
            std::string protocolName = protocol[JSON_PROT_NAME].GetString ();

            if (protocolName != PROTOCOL_IEC61850)
            {
                continue;
            }
            if (!protocol.HasMember (JSON_PROT_OBJ_REF)
                || !protocol[JSON_PROT_OBJ_REF].IsString ())
            {
                Iec61850Utility::log_error (
                    "PROTOCOL HAS NO OBJECT REFERENCE");
                return;
            }
            if (!protocol.HasMember (JSON_PROT_CDC)
                || !protocol[JSON_PROT_CDC].IsString ())
            {
                Iec61850Utility::log_error ("PROTOCOL HAS NO CDC");
                return;
            }

            const std::string objRef
                = protocol[JSON_PROT_OBJ_REF].GetString ();
            const std::string typeIdStr = protocol[JSON_PROT_CDC].GetString ();

            Iec61850Utility::log_info ("  address: %s type: %s label: %s \n ",
                                       objRef.c_str (), typeIdStr.c_str (),
                                       label.c_str ());

            int typeId = getCdcTypeFromString (typeIdStr);

            if (typeId == -1)
            {
                Iec61850Utility::log_error ("Invalid CDC type, skip",
                                            typeIdStr.c_str ());
                continue;
            }

            auto cdcType = static_cast<CDCTYPE> (typeId);

            auto it = m_exchangeDefinitions.find (label);

            if (it != m_exchangeDefinitions.end ())
            {
                Iec61850Utility::log_warn ("DataExchangeDefinition with label "
                                           "%s already exists -> ignore",
                                           label.c_str ());
                continue;
            }

            auto def = std::make_shared<DataExchangeDefinition> ();

            def->objRef = objRef;
            def->cdcType = cdcType;
            def->label = label;
            def->id = pivot_id;

            m_exchangeDefinitions.insert ({ label, def });
            m_exchangeDefinitionsPivotId.insert ({ pivot_id, def });
            m_exchangeDefinitionsObjRef.insert ({ objRef, def });
            m_polledDatapoints.insert ({ objRef, def });
        }
    }
}

std::shared_ptr<DataExchangeDefinition>
IEC61850ClientConfig::getExchangeDefinitionByObjRef (const std::string& objRef)
{
    auto it = m_exchangeDefinitionsObjRef.find (objRef);
    if (it != m_exchangeDefinitionsObjRef.end ())
    {
        return it->second;
    }
    return nullptr;
}

void
IEC61850ClientConfig::importTlsConfig (const std::string& tlsConfig)
{
    Document document;

    if (document.Parse (const_cast<char*> (tlsConfig.c_str ()))
            .HasParseError ())
    {
        Iec61850Utility::log_fatal ("Parsing error in TLS configuration");

        return;
    }

    if (!document.IsObject ())
        return;

    if (!document.HasMember ("tls_conf") || !document["tls_conf"].IsObject ())
    {
        return;
    }

    const Value& tlsConf = document["tls_conf"];

    if (tlsConf.HasMember ("private_key")
        && tlsConf["private_key"].IsString ())
    {
        m_privateKey = tlsConf["private_key"].GetString ();
    }

    if (tlsConf.HasMember ("own_cert") && tlsConf["own_cert"].IsString ())
    {
        m_ownCertificate = tlsConf["own_cert"].GetString ();
    }

    if (tlsConf.HasMember ("ca_certs") && tlsConf["ca_certs"].IsArray ())
    {

        const Value& caCerts = tlsConf["ca_certs"];

        for (const Value& caCert : caCerts.GetArray ())
        {
            if (caCert.HasMember ("cert_file"))
            {
                if (caCert["cert_file"].IsString ())
                {
                    std::string certFileName
                        = caCert["cert_file"].GetString ();

                    m_caCertificates.push_back (certFileName);
                }
            }
        }
    }

    if (tlsConf.HasMember ("remote_certs")
        && tlsConf["remote_certs"].IsArray ())
    {
        const Value& remoteCerts = tlsConf["remote_certs"];

        for (const Value& remoteCert : remoteCerts.GetArray ())
        {
            if (remoteCert.HasMember ("cert_file"))
            {
                if (remoteCert["cert_file"].IsString ())
                {
                    std::string certFileName
                        = remoteCert["cert_file"].GetString ();

                    m_remoteCertificates.push_back (certFileName);
                }
            }
        }
    }
}
