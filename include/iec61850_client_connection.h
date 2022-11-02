#ifndef INCLUDE_IEC61850_CLIENT_CONNECTION_H_
#define INCLUDE_IEC61850_CLIENT_CONNECTION_H_

/*
 * Fledge IEC 61850 south plugin.
 *
 * Copyright (c) 2022, RTE (https://www.rte-france.com)
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Mikael Bourhis-Cloarec
 */

#include <memory>
#include <mutex>   // NOLINT

// libiec61850 headers
#include <libiec61850/iec61850_client.h>

// Fledge headers
#include <logger.h>

// local library
#include "./iec61850_client_config.h"

class IEC61850Client;


/** \class Mms
 *  \brief Encapsulate an MmsValue pointer
 *
 *  Encapsulate an MmsValue pointer for automatically deleting the
 *  allocated memory at the end of the life cycle of this object
 */
class Mms
{
    public:
        Mms() = default;
        ~Mms();

        /** Disable copy constructor */
        Mms(const Mms &) = delete;
        /** Disable copy assignment operator */
        Mms &operator = (const Mms &) = delete;
        /** Disable move constructor */
        Mms(Mms &&) = delete;
        /** Disable move assignment operator */
        Mms &operator = (Mms &&) = delete;

        void setMmsValue(MmsValue *mmsValue);
        const MmsValue *getMmsValue() const;
        bool isNull() const;

    private:
        MmsValue *m_mmsValue = nullptr;
};

/** \class IEC61850ClientConnection
 *  \brief Create and use a IEC61850 connection with a IED
 *
 *  Wrap a libiec61850 'IedConnection' object for
 *  opening connection with a distant IED,
 *  sending 'operation' or 'reading' order,
 *  receiving asynchronous reports
 */
class IEC61850ClientConnection
{
    public :

        explicit
        IEC61850ClientConnection(const ServerConnectionParameters &connParam);

        ~IEC61850ClientConnection();

        /** Disable default constructor */
        IEC61850ClientConnection() = delete;

        /** Disable copy constructor */
        IEC61850ClientConnection(const IEC61850ClientConnection &) = delete;
        /** Disable copy assignment operator */
        IEC61850ClientConnection &operator = (const IEC61850ClientConnection &) = delete;
        /** Disable move constructor */
        IEC61850ClientConnection(IEC61850ClientConnection &&) = delete;
        /** Disable move assignment operator */
        IEC61850ClientConnection &operator = (IEC61850ClientConnection &&) = delete;

        bool isConnected();
        bool isNoError() const;
        void logError() const;

        std::shared_ptr<Mms> readMms(const std::string &daPath,
                                     const std::string &fcName);

    private:
        void open();
        void close();

        ServerConnectionParameters m_connectionParam;
        std::mutex m_iedConnectionMutex; // libiec61850 thread safe?: protect the IedConnection

        // libiec61850 objects
        IedConnection       m_iedConnection = nullptr;
        IedClientError      m_networkStack_error = IED_ERROR_OK;
};

#endif  // INCLUDE_IEC61850_CLIENT_CONNECTION_H_
