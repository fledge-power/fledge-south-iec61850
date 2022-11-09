/*
 * Fledge IEC 61850 south plugin.
 *
 * Copyright (c) 2022, RTE (https://www.rte-france.com)
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Mikael Bourhis-Cloarec
 */

#include "./iec61850_client_connection.h"

// libiec61850 headers
#include <libiec61850/iec61850_common.h>

IEC61850ClientConnection::IEC61850ClientConnection(
    const ServerConnectionParameters &connParam)
    : m_connectionParam(connParam)
{
    Logger::getLogger()->debug("IEC61850ClientConn: constructor");
    m_iedConnection = IedConnection_create();
    open();
}

IEC61850ClientConnection::~IEC61850ClientConnection()
{
    Logger::getLogger()->debug("IEC61850ClientConn: destructor");
    close();
    IedConnection_destroy(m_iedConnection);
}

bool IEC61850ClientConnection::isNoError() const
{
    return (m_networkStack_error == IED_ERROR_OK);
}

bool IEC61850ClientConnection::isConnected()
{
    std::unique_lock<std::mutex> connectionGuard(m_iedConnectionMutex);
    return (IedConnection_getState(m_iedConnection) == IED_STATE_CONNECTED);
}

void IEC61850ClientConnection::open()
{
    Logger::getLogger()->debug("IEC61850ClientConn: open");
    std::unique_lock<std::mutex> connectionGuard(m_iedConnectionMutex);
    IedConnection_connect(m_iedConnection,
                          &m_networkStack_error,
                          m_connectionParam.ipAddress.c_str(),
                          m_connectionParam.mmsPort);
}

void IEC61850ClientConnection::close()
{
    Logger::getLogger()->debug("IEC61850ClientConn: close");
    std::unique_lock<std::mutex> connectionGuard(m_iedConnectionMutex);
    IedConnection_close(m_iedConnection);
}

std::shared_ptr<WrappedMms>
IEC61850ClientConnection::readMms(const std::string &daPath,
                                  const std::string &fcName)
{
    // Preconditions
    if (! isConnected()) {
        return nullptr;
    }

    FunctionalConstraint functionalConstraint =
        FunctionalConstraint_fromString(fcName.c_str());
    auto wrapped_mms = std::make_shared<WrappedMms>();
    std::unique_lock<std::mutex> connectionGuard(m_iedConnectionMutex);
    wrapped_mms->setMmsValue(IedConnection_readObject(m_iedConnection,
                             &m_networkStack_error,
                             daPath.c_str(),
                             functionalConstraint));
    return wrapped_mms;
}


void IEC61850ClientConnection::logError() const
{
    switch (m_networkStack_error) {
        case IED_ERROR_OK:
            Logger::getLogger()->debug("No error occurred");
            break;

        case IED_ERROR_NOT_CONNECTED:
            Logger::getLogger()->error("The client is not yet connected");
            break;

        case IED_ERROR_ALREADY_CONNECTED:
            Logger::getLogger()->error("Connect service not execute because the client is already connected");
            break;

        case IED_ERROR_CONNECTION_LOST:
            Logger::getLogger()->error("The service request can not be executed caused by a loss of connection");
            break;

        case IED_ERROR_SERVICE_NOT_SUPPORTED:
            Logger::getLogger()->error("The service or some given parameters are not supported by the client stack or by the server");
            break;

        case IED_ERROR_CONNECTION_REJECTED:
            Logger::getLogger()->error("Connection rejected by server");
            break;

        case IED_ERROR_OUTSTANDING_CALL_LIMIT_REACHED:
            Logger::getLogger()->error("Cannot send request because outstanding call limit is reached");
            break;

        case IED_ERROR_USER_PROVIDED_INVALID_ARGUMENT:
            Logger::getLogger()->error("API function has been called with an invalid argument");
            break;

        case IED_ERROR_ENABLE_REPORT_FAILED_DATASET_MISMATCH:
            Logger::getLogger()->error("Enable report failed dataset mismatch");
            break;

        case IED_ERROR_OBJECT_REFERENCE_INVALID:
            Logger::getLogger()->error("The object provided object reference is invalid (there is a syntactical error)");
            break;

        case IED_ERROR_UNEXPECTED_VALUE_RECEIVED:
            Logger::getLogger()->error("Received object is of unexpected type");
            break;

        case IED_ERROR_TIMEOUT:
            Logger::getLogger()->error("The communication to the server failed with a timeout");
            break;

        case IED_ERROR_ACCESS_DENIED:
            Logger::getLogger()->error("The server rejected the access to the requested object/service due to access control");
            break;

        case IED_ERROR_OBJECT_DOES_NOT_EXIST:
            Logger::getLogger()->error("The server reported that the requested object does not exist (returned by server)");
            break;

        case IED_ERROR_OBJECT_EXISTS:
            Logger::getLogger()->error("The server reported that the requested object already exists");
            break;

        case IED_ERROR_OBJECT_ACCESS_UNSUPPORTED:
            Logger::getLogger()->error("The server does not support the requested access method (returned by server)");
            break;

        case IED_ERROR_TYPE_INCONSISTENT:
            Logger::getLogger()->error("The server expected an object of another type (returned by server)");
            break;

        case IED_ERROR_TEMPORARILY_UNAVAILABLE:
            Logger::getLogger()->error("The object or service is temporarily unavailable (returned by server)");
            break;

        case IED_ERROR_OBJECT_UNDEFINED:
            Logger::getLogger()->error("The specified object is not defined in the server (returned by server)");
            break;

        case IED_ERROR_INVALID_ADDRESS:
            Logger::getLogger()->error("The specified address is invalid (returned by server)");
            break;

        case IED_ERROR_HARDWARE_FAULT:
            Logger::getLogger()->error("Service failed due to a hardware fault (returned by server)");
            break;

        case IED_ERROR_TYPE_UNSUPPORTED:
            Logger::getLogger()->error("The requested data type is not supported by the server (returned by server)");
            break;

        case IED_ERROR_OBJECT_ATTRIBUTE_INCONSISTENT:
            Logger::getLogger()->error("The provided attributes are inconsistent (returned by server)");
            break;

        case IED_ERROR_OBJECT_VALUE_INVALID:
            Logger::getLogger()->error("The provided object value is invalid (returned by server)");
            break;

        case IED_ERROR_OBJECT_INVALIDATED:
            Logger::getLogger()->error("The object is invalidated (returned by server)");
            break;

        case IED_ERROR_MALFORMED_MESSAGE:
            Logger::getLogger()->error("Received an invalid response message from the server");
            break;

        case IED_ERROR_SERVICE_NOT_IMPLEMENTED:
            Logger::getLogger()->error("Service not implemented");
            break;

        default:
            Logger::getLogger()->error("unknown error");
            break;
    }
}
