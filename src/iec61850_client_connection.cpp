#include "iec61850_client_connection.hpp"
#include "iec61850_client_config.hpp"
#include <algorithm>
#include <iec61850.hpp>
#include <libiec61850/hal_thread.h>
#include <libiec61850/iec61850_client.h>
#include <libiec61850/mms_value.h>
#include <map>
#include <string>
#include <utils.h>
#include <vector>

IEC61850ClientConnection::IEC61850ClientConnection (
    IEC61850Client* client, IEC61850ClientConfig* config,
    const std::string& ip, const int tcpPort, bool tls,
    OsiParameters* osiParameters)
    : m_client (client), m_config (config), m_osiParameters (osiParameters),
      m_tcpPort (tcpPort), m_serverIp (ip), m_useTls (tls)
{
}

IEC61850ClientConnection::~IEC61850ClientConnection () { Stop (); }

static uint64_t
getMonotonicTimeInMs ()
{
    uint64_t timeVal = 0;

    struct timespec ts;

    if (clock_gettime (CLOCK_MONOTONIC, &ts) == 0)
    {
        timeVal = ((uint64_t)ts.tv_sec * 1000LL) + (ts.tv_nsec / 1000000);
    }

    return timeVal;
}

void
IEC61850ClientConnection::commandTerminationHandler (
    void* parameter, ControlObjectClient connection)
{
    LastApplError lastApplError
        = ControlObjectClient_getLastApplError (connection);
    if (lastApplError.error != CONTROL_ERROR_NO_ERROR)
    {
        logControlErrors (
            lastApplError.addCause, lastApplError.error,
            std::string (ControlObjectClient_getObjectReference (connection)));
        Iec61850Utility::log_error ("Couldn't terminate command");
        return;
    }

    auto connectionCosPair
        = (std::pair<IEC61850ClientConnection*, ControlObjectStruct*>*)
            parameter;
    IEC61850ClientConnection* con = connectionCosPair->first;
    ControlObjectStruct* cos = connectionCosPair->second;

    cos->state = CONTROL_IDLE;
    con->sendActTerm (cos);
}

// LCOV_EXCL_START
void
IEC61850ClientConnection::logControlErrors (ControlAddCause addCause,
                                            ControlLastApplError lastApplError,
                                            const std::string& info)
{
    Iec61850Utility::log_error ("In here : %s", info.c_str ());
    switch (addCause)
    {
    case ADD_CAUSE_UNKNOWN:
        Iec61850Utility::log_error ("Unknown add cause");
        break;
    case ADD_CAUSE_NOT_SUPPORTED:
        Iec61850Utility::log_error ("Add cause not supported");
        break;
    case ADD_CAUSE_BLOCKED_BY_SWITCHING_HIERARCHY:
        Iec61850Utility::log_error ("Blocked by switching hierarchy");
        break;
    case ADD_CAUSE_SELECT_FAILED:
        Iec61850Utility::log_error ("Select failed");
        break;
    case ADD_CAUSE_INVALID_POSITION:
        Iec61850Utility::log_error ("Invalid position");
        break;
    case ADD_CAUSE_POSITION_REACHED:
        Iec61850Utility::log_error ("Position reached");
        break;
    case ADD_CAUSE_PARAMETER_CHANGE_IN_EXECUTION:
        Iec61850Utility::log_error ("Parameter change in execution");
        break;
    case ADD_CAUSE_STEP_LIMIT:
        Iec61850Utility::log_error ("Step limit reached");
        break;
    case ADD_CAUSE_BLOCKED_BY_MODE:
        Iec61850Utility::log_error ("Blocked by mode");
        break;
    case ADD_CAUSE_BLOCKED_BY_PROCESS:
        Iec61850Utility::log_error ("Blocked by process");
        break;
    case ADD_CAUSE_BLOCKED_BY_INTERLOCKING:
        Iec61850Utility::log_error ("Blocked by interlocking");
        break;
    case ADD_CAUSE_BLOCKED_BY_SYNCHROCHECK:
        Iec61850Utility::log_error ("Blocked by synchrocheck");
        break;
    case ADD_CAUSE_COMMAND_ALREADY_IN_EXECUTION:
        Iec61850Utility::log_error ("Command already in execution");
        break;
    case ADD_CAUSE_BLOCKED_BY_HEALTH:
        Iec61850Utility::log_error ("Blocked by health status");
        break;
    case ADD_CAUSE_1_OF_N_CONTROL:
        Iec61850Utility::log_error ("1 of N control error");
        break;
    case ADD_CAUSE_ABORTION_BY_CANCEL:
        Iec61850Utility::log_error ("Aborted by cancel");
        break;
    case ADD_CAUSE_TIME_LIMIT_OVER:
        Iec61850Utility::log_error ("Time limit exceeded");
        break;
    case ADD_CAUSE_ABORTION_BY_TRIP:
        Iec61850Utility::log_error ("Aborted by trip");
        break;
    case ADD_CAUSE_OBJECT_NOT_SELECTED:
        Iec61850Utility::log_error ("Object not selected");
        break;
    case ADD_CAUSE_OBJECT_ALREADY_SELECTED:
        Iec61850Utility::log_error ("Object already selected");
        break;
    case ADD_CAUSE_NO_ACCESS_AUTHORITY:
        Iec61850Utility::log_error ("No access authority");
        break;
    case ADD_CAUSE_ENDED_WITH_OVERSHOOT:
        Iec61850Utility::log_error ("Ended with overshoot");
        break;
    case ADD_CAUSE_ABORTION_DUE_TO_DEVIATION:
        Iec61850Utility::log_error ("Aborted due to deviation");
        break;
    case ADD_CAUSE_ABORTION_BY_COMMUNICATION_LOSS:
        Iec61850Utility::log_error ("Aborted by communication loss");
        break;
    case ADD_CAUSE_ABORTION_BY_COMMAND:
        Iec61850Utility::log_error ("Aborted by command");
        break;
    case ADD_CAUSE_NONE:
        Iec61850Utility::log_info ("No add cause error");
        break;
    case ADD_CAUSE_INCONSISTENT_PARAMETERS:
        Iec61850Utility::log_error ("Inconsistent parameters");
        break;
    case ADD_CAUSE_LOCKED_BY_OTHER_CLIENT:
        Iec61850Utility::log_error ("Locked by another client");
        break;
    }

    switch (lastApplError)
    {
    case CONTROL_ERROR_NO_ERROR:
        Iec61850Utility::log_info ("No last application error");
        break;
    case CONTROL_ERROR_UNKNOWN:
        Iec61850Utility::log_error ("Unknown last application error");
        break;
    case CONTROL_ERROR_TIMEOUT_TEST:
        Iec61850Utility::log_error ("Timeout test error");
        break;
    case CONTROL_ERROR_OPERATOR_TEST:
        Iec61850Utility::log_error ("Operator test error");
        break;
    }
}
// LCOV_EXCL_STOP

void
IEC61850ClientConnection::m_setOsiConnectionParameters ()
{
    MmsConnection mmsConnection
        = IedConnection_getMmsConnection (m_connection);
    IsoConnectionParameters libiecIsoParams
        = MmsConnection_getIsoConnectionParameters (mmsConnection);
    const OsiParameters& osiParams = *m_osiParameters;

    // set Remote 'AP Title' and 'AE Qualifier'
    if (!osiParams.remoteApTitle.empty ())
    {
        IsoConnectionParameters_setRemoteApTitle (
            libiecIsoParams, osiParams.remoteApTitle.c_str (),
            osiParams.remoteAeQualifier);
    }

    // set Local 'AP Title' and 'AE Qualifier'
    if (!osiParams.localApTitle.empty ())
    {
        IsoConnectionParameters_setLocalApTitle (
            libiecIsoParams, osiParams.localApTitle.c_str (),
            osiParams.localAeQualifier);
    }

    /* change parameters for presentation, session and transport layers */
    IsoConnectionParameters_setRemoteAddresses (
        libiecIsoParams, osiParams.remotePSelector, osiParams.remoteSSelector,
        osiParams.localTSelector);
    IsoConnectionParameters_setLocalAddresses (
        libiecIsoParams, osiParams.localPSelector, osiParams.localSSelector,
        osiParams.remoteTSelector);
}

void
IEC61850ClientConnection::m_configDatasets ()
{
    for (const auto& pair : m_config->getDatasets ())
    {
        IedClientError error;
        std::shared_ptr<Dataset> dataset = pair.second;

        if (dataset->dynamic)
        {
            Iec61850Utility::log_debug ("Create new dataset %s",
                                        dataset->datasetRef.c_str ());
            LinkedList newDataSetEntries = LinkedList_create ();

            if (newDataSetEntries == nullptr)
            {
                continue;
            }

            for (const auto& entry : dataset->entries)
            {
                char* strCopy
                    = static_cast<char*> (malloc (entry.length () + 1));
                if (strCopy != nullptr)
                {
                    std::strcpy (strCopy, entry.c_str ());
                    LinkedList_add (newDataSetEntries,
                                    static_cast<void*> (strCopy));
                }
            }

            IedConnection_createDataSet (m_connection, &error,
                                         dataset->datasetRef.c_str (),
                                         newDataSetEntries);

            if (error != IED_ERROR_OK)
            {
                m_client->logIedClientError (error, "Create Dataset");
            }

            LinkedList_destroyDeep (newDataSetEntries, free);
        }
    }
}

void
IEC61850ClientConnection::writeVariableHandler (uint32_t invokeId,
                                                void* parameter, MmsError err,
                                                MmsDataAccessError accessError)
{
    IedConnection self = (IedConnection)parameter;
}

void
IEC61850ClientConnection::reportCallbackFunction (void* parameter,
                                                  ClientReport report)
{
    auto pair = (std::pair<IEC61850ClientConnection*, LinkedList>*)parameter;
    IEC61850ClientConnection* con = pair->first;
    LinkedList dataSetDirectory = pair->second;

    MmsValue const* dataSetValues = ClientReport_getDataSetValues (report);

    Iec61850Utility::log_debug ("received report for %s with rptId %s\n",
                                ClientReport_getRcbReference (report),
                                ClientReport_getRptId (report));

    time_t unixTime = 0;

    if (ClientReport_hasTimestamp (report))
    {
        unixTime = ClientReport_getTimestamp (report) / 1000;

        Iec61850Utility::log_debug ("  report contains timestamp (%u)",
                                    (unsigned int)unixTime);
    }

    if (!dataSetDirectory)
        return;

    for (int i = 0; i < LinkedList_size (dataSetDirectory); i++)
    {
        ReasonForInclusion reason
            = ClientReport_getReasonForInclusion (report, i);

        if (reason == IEC61850_REASON_NOT_INCLUDED)
            continue;

        LinkedList entry = LinkedList_get (dataSetDirectory, i);

        auto* entryName = (char*)entry->data;
        if (!dataSetValues)
            continue;
        MmsValue* value = MmsValue_getElement (dataSetValues, i);
        if (!value)
            continue;

        Iec61850Utility::log_debug ("%s (included for reason %i)", entryName,
                                    reason);

        con->m_client->handleValue (std::string (entryName), value, unixTime);
    }
}

static int
configureRcb (const std::shared_ptr<ReportSubscription>& rs,
              ClientReportControlBlock rcb)
{
    uint32_t parametersMask = 0;

    bool isBuffered = ClientReportControlBlock_isBuffered (rcb);

    if (isBuffered)
        parametersMask |= RCB_ELEMENT_RESV_TMS;
    else
        parametersMask |= RCB_ELEMENT_RESV;

    if (rs->trgops != -1)
    {
        parametersMask |= RCB_ELEMENT_TRG_OPS;
        ClientReportControlBlock_setTrgOps (rcb, rs->trgops);
    }
    if (rs->buftm != -1)
    {
        parametersMask |= RCB_ELEMENT_BUF_TM;
        ClientReportControlBlock_setBufTm (rcb, rs->buftm);
    }
    if (rs->intgpd != -1)
    {
        parametersMask |= RCB_ELEMENT_INTG_PD;
        ClientReportControlBlock_setIntgPd (rcb, rs->intgpd);
    }
    if (rs->gi)
    {
        parametersMask |= RCB_ELEMENT_GI;
        ClientReportControlBlock_setGI (rcb, rs->gi);
    }

    if (!rs->datasetRef.empty ())
    {
        parametersMask |= RCB_ELEMENT_DATSET;
        std::string modifiedDataSetRef = rs->datasetRef;
        std::replace (modifiedDataSetRef.begin (), modifiedDataSetRef.end (),
                      '.', '$');
        ClientReportControlBlock_setDataSetReference (
            rcb, modifiedDataSetRef.c_str ());
    }

    ClientReportControlBlock_setRptEna (rcb, true);
    parametersMask |= RCB_ELEMENT_RPT_ENA;

    return parametersMask;
}

void
IEC61850ClientConnection::m_configRcb ()
{
    for (const auto& pair : m_config->getReportSubscriptions ())
    {
        IedClientError error;
        std::shared_ptr<ReportSubscription> rs = pair.second;
        ClientReportControlBlock rcb = nullptr;
        ClientDataSet clientDataSet = nullptr;
        LinkedList dataSetDirectory = nullptr;

        std::stringstream ss;
        ss << "reportsubscription - rcbref: " << rs->rcbRef
           << ", datasetref: " << rs->datasetRef << ", trgops: " << rs->trgops
           << ", buftm: " << rs->buftm << ", intgpd: " << rs->intgpd;
        Iec61850Utility::log_debug ("%s", ss.str ().c_str ());

        dataSetDirectory = IedConnection_getDataSetDirectory (
            m_connection, &error, rs->datasetRef.c_str (), nullptr);

        if (error != IED_ERROR_OK)
        {
            Iec61850Utility::log_error (
                "Reading data set directory failed!\n");
            continue;
        }

        clientDataSet = IedConnection_readDataSetValues (
            m_connection, &error, rs->datasetRef.c_str (), nullptr);

        if (clientDataSet == nullptr)
        {
            Iec61850Utility::log_error ("Failed to read dataset\n");
            continue;
        }

        rcb = IedConnection_getRCBValues (
            m_connection, &error, rs->rcbRef.c_str (), nullptr);

        if (error != IED_ERROR_OK)
        {
            Iec61850Utility::log_error ("GetRCBValues service error!\n");
            continue;
        }

        uint32_t parametersMask = configureRcb (rs, rcb);

        auto connDataSetPair
            = new std::pair<IEC61850ClientConnection*, LinkedList> (
                this, dataSetDirectory);
        m_connDataSetDirectoryPairs.push_back (connDataSetPair);

        IedConnection_installReportHandler (
            m_connection, (rs->rcbRef.substr(0,rs->rcbRef.size()-2)).c_str (),
            ClientReportControlBlock_getRptId (rcb), reportCallbackFunction,
            static_cast<void*> (connDataSetPair));

        IedConnection_setRCBValues (m_connection, &error, rcb, parametersMask,
                                    true);

        if (clientDataSet)
            ClientDataSet_destroy (clientDataSet);

        if (rcb)
            ClientReportControlBlock_destroy (rcb);

        if (error != IED_ERROR_OK)
        {
            m_client->logIedClientError (error, "Set RCB Values");
            return;
        }
    }
}

void
IEC61850ClientConnection::m_setVarSpecs ()
{
    IedClientError err;
    for (const auto& entry : m_config->ExchangeDefinition ())
    {
        auto def = entry.second;
        FunctionalConstraint fc = def->cdcType == MV || def->cdcType == APC
                                      ? IEC61850_FC_MX
                                      : IEC61850_FC_ST;
        MmsVariableSpecification* spec
            = getVariableSpec (&err, def->objRef.c_str (), fc);
        if (spec)
        {
            def->spec = spec;
        }
    }
}

void
IEC61850ClientConnection::m_initialiseControlObjects ()
{
    for (const auto& entry : m_config->ExchangeDefinition ())
    {
        auto def = entry.second;
        if (def->cdcType < SPC || def->cdcType >= SPG)
            continue;
        IedClientError err;
        MmsValue* temp = IedConnection_readObject (
            m_connection, &err, def->objRef.c_str (), IEC61850_FC_ST);
        if (err != IED_ERROR_OK)
        {
            m_client->logIedClientError (err, "Initialise control object");
            continue;
        }
        MmsValue_delete (temp);
        auto co = new ControlObjectStruct;
        co->client
            = ControlObjectClient_create (def->objRef.c_str (), m_connection);
        co->mode = ControlObjectClient_getControlModel (co->client);
        co->state = CONTROL_IDLE;
        co->label = entry.first;
        switch (def->cdcType)
        {
        case SPC:
        case DPC: {
            co->value = MmsValue_newBoolean (false);
            break;
        }
        case BSC: {
            co->value = MmsValue_newBitString (2);
            break;
        }
        case APC: {
            co->value = MmsValue_newFloat (0.0);
            break;
        }
        case INC: {
            co->value = MmsValue_newIntegerFromInt32 (0);
            break;
        }
        default: {
            Iec61850Utility::log_error ("Invalid cdc type");
            return;
        }
        }
        Iec61850Utility::log_debug ("Added control object %s , %s ",
                                    co->label.c_str (), def->objRef.c_str ());
        m_controlObjects.insert ({ def->objRef, co });
    }
}

void
IEC61850ClientConnection::Start ()
{
    if (!m_started)
    {
        m_started = true;

        m_conThread
            = new std::thread (&IEC61850ClientConnection::_conThread, this);
    }
}

void
IEC61850ClientConnection::cleanUp ()
{
    if (!m_config->ExchangeDefinition ().empty ())
    {
        for (const auto& def : m_config->ExchangeDefinition ())
        {
            if (def.second->spec)
            {
                MmsVariableSpecification_destroy (def.second->spec);
                def.second->spec = nullptr;
            }
        }
    }

    if (!m_connDataSetDirectoryPairs.empty ())
    {
        for (const auto& entry : m_connDataSetDirectoryPairs)
        {
            LinkedList dataSetDirectory = entry->second;
            LinkedList_destroy (dataSetDirectory);
            delete entry;
        }
        m_connDataSetDirectoryPairs.clear ();
    }

    if (!m_controlObjects.empty ())
    {
        for (auto& co : m_controlObjects)
        {
            ControlObjectStruct* cos = co.second;
            if (cos)
            {
                if (cos->client)
                {
                    if (cos->client)
                        ControlObjectClient_destroy (cos->client);
                    cos->client = nullptr;
                }
                if (cos->value)
                {
                    MmsValue_delete (cos->value);
                    cos->value = nullptr;
                }
                delete cos;
            }
        }
        m_controlObjects.clear ();
    }

    if (!m_connControlPairs.empty ())
    {
        for (auto& cc : m_connControlPairs)
        {
            delete cc;
        }
        m_connControlPairs.clear ();
    }

    IedClientError err;

    if (m_connection)
    {
        IedConnection_close (m_connection);
        IedConnection_abortAsync (m_connection, &err);
        IedConnection_destroy (m_connection);
        m_connection = nullptr;
    }

    if (m_tlsConfig != nullptr)
    {
        TLSConfiguration_destroy (m_tlsConfig);
    }
    m_tlsConfig = nullptr;
}

void
IEC61850ClientConnection::Stop ()
{
    if (!m_started)
        return;

    {
        std::lock_guard<std::mutex> lock (m_conLock);
        m_started = false;
    }
    if (m_conThread)
    {
        m_conThread->join ();
        delete m_conThread;
        m_conThread = nullptr;
    }
}

bool
IEC61850ClientConnection::prepareConnection ()
{
    if (UseTLS ())
    {
        TLSConfiguration tlsConfig = TLSConfiguration_create ();

        bool tlsConfigOk = true;

        std::string certificateStore
            = getDataDir () + std::string ("/etc/certs/");
        std::string certificateStorePem
            = getDataDir () + std::string ("/etc/certs/pem/");

        if (m_config->GetOwnCertificate ().length () == 0
            || m_config->GetPrivateKey ().length () == 0)
        {
            Iec61850Utility::log_error (
                "No private key and/or certificate configured for client");
            tlsConfigOk = false;
        }
        else
        {
            std::string privateKeyFile
                = certificateStore + m_config->GetPrivateKey ();

            if (access (privateKeyFile.c_str (), R_OK) == 0)
            {
                if (TLSConfiguration_setOwnKeyFromFile (
                        tlsConfig, privateKeyFile.c_str (), nullptr)
                    == false)
                {
                    Iec61850Utility::log_error (
                        "Failed to load private key file: %s",
                        privateKeyFile.c_str ());
                    tlsConfigOk = false;
                }
            }
            else
            {
                Iec61850Utility::log_error (
                    "Failed to access private key file: %s",
                    privateKeyFile.c_str ());
                tlsConfigOk = false;
            }

            std::string clientCert = m_config->GetOwnCertificate ();
            bool isPemClientCertificate
                = clientCert.rfind (".pem") == clientCert.size () - 4;

            std::string clientCertFile;

            if (isPemClientCertificate)
                clientCertFile = certificateStorePem + clientCert;
            else
                clientCertFile = certificateStore + clientCert;

            if (access (clientCertFile.c_str (), R_OK) == 0)
            {
                if (TLSConfiguration_setOwnCertificateFromFile (
                        tlsConfig, clientCertFile.c_str ())
                    == false)
                {
                    Iec61850Utility::log_error (
                        "Failed to load client certificate file: %s",
                        clientCertFile.c_str ());
                    tlsConfigOk = false;
                }
            }
            else
            {
                Iec61850Utility::log_error (
                    "Failed to access client certificate file: %s",
                    clientCertFile.c_str ());
                tlsConfigOk = false;
            }
        }

        if (!m_config->GetRemoteCertificates ().empty ())
        {
            TLSConfiguration_setAllowOnlyKnownCertificates (tlsConfig, true);

            for (const std::string& remoteCert :
                 m_config->GetRemoteCertificates ())
            {
                bool isPemRemoteCertificate
                    = remoteCert.rfind (".pem") == remoteCert.size () - 4;

                std::string remoteCertFile;

                if (isPemRemoteCertificate)
                    remoteCertFile = certificateStorePem + remoteCert;
                else
                    remoteCertFile = certificateStore + remoteCert;

                if (access (remoteCertFile.c_str (), R_OK) == 0)
                {
                    if (TLSConfiguration_addAllowedCertificateFromFile (
                            tlsConfig, remoteCertFile.c_str ())
                        == false)
                    {
                        Iec61850Utility::log_warn (
                            "Failed to load remote certificate file: %s -> "
                            "ignore certificate",
                            remoteCertFile.c_str ());
                    }
                }
                else
                {
                    Iec61850Utility::log_warn (
                        "Failed to access remote certificate file: %s -> "
                        "ignore certificate",
                        remoteCertFile.c_str ());
                }
            }
        }
        else
        {
            TLSConfiguration_setAllowOnlyKnownCertificates (tlsConfig, false);
        }

        if (m_config->GetCaCertificates ().size () > 0)
        {
            TLSConfiguration_setChainValidation (tlsConfig, true);

            for (const std::string& caCert : m_config->GetCaCertificates ())
            {
                bool isPemCaCertificate
                    = caCert.rfind (".pem") == caCert.size () - 4;

                std::string caCertFile;

                if (isPemCaCertificate)
                    caCertFile = certificateStorePem + caCert;
                else
                    caCertFile = certificateStore + caCert;

                if (access (caCertFile.c_str (), R_OK) == 0)
                {
                    if (TLSConfiguration_addCACertificateFromFile (
                            tlsConfig, caCertFile.c_str ())
                        == false)
                    {
                        Iec61850Utility::log_warn (
                            "Failed to load CA certificate file: %s -> ignore "
                            "certificate",
                            caCertFile.c_str ());
                    }
                }
                else
                {
                    Iec61850Utility::log_warn (
                        "Failed to access CA certificate file: %s -> ignore "
                        "certificate",
                        caCertFile.c_str ());
                }
            }
        }
        else
        {
            TLSConfiguration_setChainValidation (tlsConfig, false);
        }

        if (tlsConfigOk)
        {

            TLSConfiguration_setRenegotiationTime (tlsConfig, 60000);

            m_connection = IedConnection_createWithTlsSupport (tlsConfig);

            if (m_connection)
            {
                m_tlsConfig = tlsConfig;
            }
            else
            {
                printf ("TLS configuration failed\n");
                TLSConfiguration_destroy (tlsConfig);
            }
        }
        else
        {
            printf ("TLS configuration failed\n");
            Iec61850Utility::log_error ("TLS configuration failed");
        }
    }
    else
    {
        m_connection = IedConnection_create ();
    }

    return m_connection != nullptr;
}

void
IEC61850ClientConnection::Disconnect ()
{
    m_connecting = false;
    m_connected = false;
    m_connect = false;
    m_connectionState = CON_STATE_IDLE;
    cleanUp ();
}

void
IEC61850ClientConnection::Connect ()
{
    m_connect = true;
}

MmsVariableSpecification*
IEC61850ClientConnection::getVariableSpec (IedClientError* error,
                                           const char* objRef,
                                           FunctionalConstraint fc)
{
    MmsVariableSpecification* varSpec
        = IedConnection_getVariableSpecification (m_connection, error, objRef,
                                                  fc);
    return varSpec;
}

MmsValue*
IEC61850ClientConnection::readValue (IedClientError* error, const char* objRef,
                                     FunctionalConstraint fc)
{
    MmsValue* value
        = IedConnection_readObject (m_connection, error, objRef, fc);
    return value;
}

MmsValue*
IEC61850ClientConnection::readDatasetValues (IedClientError* error,
                                             const char* datasetRef)
{
    ClientDataSet dataset = IedConnection_readDataSetValues (
        m_connection, error, datasetRef, nullptr);
    if (*error == IED_ERROR_OK)
    {
        ClientDataSet_destroy (dataset);
        return ClientDataSet_getValues (dataset);
    }
    return nullptr;
}

void
IEC61850ClientConnection::controlActionHandler (uint32_t invokeId,
                                                void* parameter,
                                                IedClientError err,
                                                ControlActionType type,
                                                bool success)
{
    if (success)
    {
        auto connectionCosPair
            = (std::pair<IEC61850ClientConnection*, ControlObjectStruct*>*)
                parameter;

        ControlObjectStruct* cos = connectionCosPair->second;

        IEC61850ClientConnection* connection = connectionCosPair->first;
        switch (type)
        {
        case CONTROL_ACTION_TYPE_OPERATE: {
            if (cos->mode == CONTROL_MODEL_SBO_ENHANCED
                || cos->mode == CONTROL_MODEL_DIRECT_ENHANCED)
            {
                cos->state = CONTROL_WAIT_FOR_ACT_TERM;
            }
            else
            {
                cos->state = CONTROL_IDLE;
            }
            connection->sendActCon (cos);
            break;
        }
        case CONTROL_ACTION_TYPE_SELECT: {
            cos->state = CONTROL_SELECTED;
            break;
        }
        case CONTROL_ACTION_TYPE_CANCEL: {
            break;
        }
        }
    }
}

void
IEC61850ClientConnection::executePeriodicTasks ()
{
    uint64_t currentTime = getMonotonicTimeInMs ();
    if (m_config->getPollingInterval () > 0
        && currentTime >= m_nextPollingTime)
    {
        m_client->handleAllValues ();
        m_nextPollingTime = currentTime + m_config->getPollingInterval ();
    }

    for (const auto& co : m_controlObjects)
    {
        ControlObjectStruct* cos = co.second;

        auto pair = std::make_shared<
            std::pair<IEC61850ClientConnection*, ControlObjectStruct*> > (this,
                                                                          cos);

        if (cos->state == CONTROL_IDLE)
            continue;
        else if (cos->state == CONTROL_SELECTED)
        {
            IedClientError error;
            ControlObjectClient_operateAsync (cos->client, &error, cos->value,
                                              0, controlActionHandler,
                                              pair.get ());
        }
    }
}

void
IEC61850ClientConnection::_conThread ()
{
    try
    {
        while (m_started)
        {
            {
                if (m_connect)
                {
                    IedConnectionState newState;
                    switch (m_connectionState)
                    {
                    case CON_STATE_IDLE:

                    {

                        if (m_connection != nullptr)
                        {
                            {
                                std::lock_guard<std::mutex> lock (m_conLock);
                                IedConnection_destroy (m_connection);
                                m_connection = nullptr;
                            }
                        }

                        if (prepareConnection ())
                        {
                            IedClientError error;
                            {
                                std::lock_guard<std::mutex> lock (m_conLock);
                                m_connectionState = CON_STATE_CONNECTING;
                                m_connecting = true;
                                m_delayExpirationTime
                                    = getMonotonicTimeInMs () + 10000;
                                if (m_osiParameters)
                                    m_setOsiConnectionParameters ();
                            }

                            IedConnection_connectAsync (m_connection, &error,
                                                        m_serverIp.c_str (),
                                                        m_tcpPort);
                            if (error == IED_ERROR_OK)
                            {
                                Iec61850Utility::log_info (
                                    "Connecting to %s:%d", m_serverIp.c_str (),
                                    m_tcpPort);
                            }
                            else
                            {
                                Iec61850Utility::log_error (
                                    "Failed to connect to %s:%d",
                                    m_serverIp.c_str (), m_tcpPort);
                                {
                                    std::lock_guard<std::mutex> lock (
                                        m_conLock);
                                    m_connectionState = CON_STATE_FATAL_ERROR;
                                }
                            }
                        }
                        else
                        {
                            {
                                std::lock_guard<std::mutex> lock (m_conLock);
                                m_connectionState = CON_STATE_FATAL_ERROR;
                            }
                            Iec61850Utility::log_error (
                                "Fatal configuration error");
                        }
                    }
                    break;

                    case CON_STATE_CONNECTING:
                        newState = IedConnection_getState (m_connection);
                        if (newState == IED_STATE_CONNECTED)
                        {
                            {
                                std::lock_guard<std::mutex> lock (m_conLock);
                                m_setVarSpecs ();
                                m_initialiseControlObjects ();
                                m_configDatasets ();
                                m_configRcb ();
                                Iec61850Utility::log_info (
                                    "Connected to %s:%d", m_serverIp.c_str (),
                                    m_tcpPort);
                                m_connectionState = CON_STATE_CONNECTED;
                                m_connecting = false;
                                m_connected = true;
                            }
                        }
                        else if (getMonotonicTimeInMs ()
                                 > m_delayExpirationTime)
                        {
                            std::lock_guard<std::mutex> lock (m_conLock);
                            Iec61850Utility::log_warn (
                                "Timeout while connecting %d", m_tcpPort);
                            Disconnect ();
                        }
                        break;

                    case CON_STATE_CONNECTED: {
                        std::lock_guard<std::mutex> lock (m_conLock);
                        newState = IedConnection_getState (m_connection);
                        if (newState != IED_STATE_CONNECTED)
                        {
                            cleanUp ();
                            m_connectionState = CON_STATE_IDLE;
                        }
                        else
                        {
                            executePeriodicTasks ();
                        }
                    }
                    break;

                    case CON_STATE_CLOSED: {
                        std::lock_guard<std::mutex> lock (m_conLock);
                        m_delayExpirationTime
                            = getMonotonicTimeInMs () + 10000;
                        m_connectionState = CON_STATE_WAIT_FOR_RECONNECT;
                    }
                    break;

                    case CON_STATE_WAIT_FOR_RECONNECT: {
                        std::lock_guard<std::mutex> lock (m_conLock);
                        if (getMonotonicTimeInMs () >= m_delayExpirationTime)
                        {
                            m_connectionState = CON_STATE_IDLE;
                        }
                    }
                    break;

                    case CON_STATE_FATAL_ERROR:
                        break;
                    }
                }
            }

            Thread_sleep (50);
        }
        {
            std::lock_guard<std::mutex> lock (m_conLock);
            cleanUp ();
        }
    }
    catch (const std::exception& e)
    {
        Iec61850Utility::log_error ("Exception caught in _conThread: %s",
                                    e.what ());
    }
}

void
IEC61850ClientConnection::sendActCon (
    const IEC61850ClientConnection::ControlObjectStruct* cos)
{
    m_client->sendCommandAck (cos->label, cos->mode, false);
}

void
IEC61850ClientConnection::sendActTerm (
    const IEC61850ClientConnection::ControlObjectStruct* cos)
{
    m_client->sendCommandAck (cos->label, cos->mode, true);
}

bool
IEC61850ClientConnection::operate (const std::string& objRef,
                                   DatapointValue value)
{
    auto it = m_controlObjects.find (objRef);

    if (it == m_controlObjects.end ())
    {
        Iec61850Utility::log_error ("Control object with objRef %s not found",
                                    objRef.c_str ());
        return false;
    }

    ControlObjectStruct* co = it->second;

    MmsValue* mmsValue = co->value;

    MmsType type = MmsValue_getType (mmsValue);

    switch (type)
    {
    case MMS_BOOLEAN:
        MmsValue_setBoolean (mmsValue, value.toInt ());
        break;
    case MMS_INTEGER:
        MmsValue_setInt32 (mmsValue, (int)value.toInt ());
        break;
    case MMS_BIT_STRING: {
        uint32_t bitStringValue = 0;
        std::string strVal = value.toStringValue ();
        if (strVal == "stop")
            bitStringValue = 0;
        else if (strVal == "lower")
            bitStringValue = 1;
        else if (strVal == "higher")
            bitStringValue = 2;
        else if (strVal == "reserved")
            bitStringValue = 3;
        MmsValue_setBitStringFromInteger (mmsValue, bitStringValue);
        break;
    }
    case MMS_FLOAT:
        MmsValue_setFloat (mmsValue, (float)value.toDouble ());
        break;
    default:
        Iec61850Utility::log_error ("Invalid mms value type");
        return false;
    }
    IedClientError error;

    auto connectionControlPair
        = new std::pair<IEC61850ClientConnection*, ControlObjectStruct*> (this,
                                                                          co);
    m_connControlPairs.push_back (connectionControlPair);

    if (co->mode == CONTROL_MODEL_DIRECT_ENHANCED
        || co->mode == CONTROL_MODEL_SBO_ENHANCED)
    {
        ControlObjectClient_setCommandTerminationHandler (
            co->client, commandTerminationHandler, connectionControlPair);
    }

    switch (co->mode)
    {
    case CONTROL_MODEL_DIRECT_ENHANCED:
    case CONTROL_MODEL_DIRECT_NORMAL:
        co->state = CONTROL_WAIT_FOR_ACT_CON;
        ControlObjectClient_operateAsync (co->client, &error, mmsValue, 0,
                                          controlActionHandler,
                                          connectionControlPair);
        break;
    case CONTROL_MODEL_SBO_NORMAL:
        co->state = CONTROL_WAIT_FOR_SELECT;
        ControlObjectClient_selectAsync (
            co->client, &error, controlActionHandler, connectionControlPair);
        break;
    case CONTROL_MODEL_SBO_ENHANCED:
        co->state = CONTROL_WAIT_FOR_SELECT_WITH_VALUE;
        ControlObjectClient_selectWithValueAsync (co->client, &error, mmsValue,
                                                  controlActionHandler,
                                                  connectionControlPair);
        break;
    case CONTROL_MODEL_STATUS_ONLY:
        break;
    }

    return true;
}

void
IEC61850ClientConnection::writeHandler (uint32_t invokeId, void* parameter,
                                        IedClientError err)
{
    auto pair = (std::pair<IEC61850ClientConnection*, MmsValue*>*) parameter;

    MmsValue* value = (MmsValue*)(pair->second);
    char valueBuffer[30];
    MmsValue_printToBuffer(value,valueBuffer,30);

    Iec61850Utility::log_debug("Write data handler called - Value: %s", valueBuffer);

    if (err != IED_ERROR_OK)
    {
        pair->first->m_client->logIedClientError(err, "Write data (Value = " + std::string(valueBuffer) + ")");
    }

    if(value){
        MmsValue_delete(value); 
    };

    delete pair;
}

bool
IEC61850ClientConnection::writeValue (Datapoint* operation, const std::string& objRef,
                                      DatapointValue value, CDCTYPE type)
{
    IedClientError err;
    MmsValue* mmsValue;
    std::string attribute;
    switch (type)
    {
    case SPG: {
        attribute = ".setVal";
        mmsValue = MmsValue_newBoolean (value.toInt ());
        Iec61850Utility::log_debug("Write value %s %d", objRef.c_str(), value.toInt());
        break;
    }
    case ING: {
        attribute = ".setVal";
        mmsValue = MmsValue_newIntegerFromInt32 ((int)value.toInt ());
        Iec61850Utility::log_debug("Write value %s %d", objRef.c_str(), value.toInt());
        break;
    }
    case ASG: {
        attribute = ".setMag.f";    
        mmsValue = MmsValue_newFloat ((float)value.toDouble ());
        Iec61850Utility::log_debug("Write value %s %f", objRef.c_str(), (float)value.toDouble());
        break;
    }
    default: {
        Iec61850Utility::log_error ("Invalid data type for writing data - %d",
                                    type);
        return false;
    }
    }


    auto parameter = new std::pair<IEC61850ClientConnection*, MmsValue*>(this,mmsValue);

    IedConnection_writeObjectAsync (m_connection, &err, (objRef + attribute).c_str (),
                                    IEC61850_FC_SP, mmsValue, writeHandler,
                                    parameter);

    delete operation;

    return err == IED_ERROR_OK;
}
