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

#include <mutex>   // NOLINT

// Fledge headers
#include <logger.h>

// local library
#include "./iec61850_client_connection_interface.h"
#include "./iec61850_client_config.h"

// For white box unit tests
#include <gtest/gtest_prod.h>

class IEC61850Client;

/** \class IEC61850ClientConnection
 *  \brief Create and use a IEC61850 connection with a IED
 *
 *  Wrap a libiec61850 'IedConnection' object for
 *  opening connection with a distant IED,
 *  sending 'operation' or 'reading' order,
 *  receiving asynchronous reports
 */
class IEC61850ClientConnection: public IEC61850ClientConnectionInterface
{
    public :

        explicit
        IEC61850ClientConnection(const ServerConnectionParameters &connParam);

        ~IEC61850ClientConnection() override;

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

        bool isConnected() override;
        bool isNoError() const override;
        void logError() const override;

        std::shared_ptr<WrappedMms> readMms(const std::string &daPath,
                                            const std::string &fcName) override;

    private:
        void open();
        void close();

        ServerConnectionParameters m_connectionParam;
        std::mutex m_iedConnectionMutex; // libiec61850 thread safe?: protect the IedConnection

        // libiec61850 objects
        IedConnection       m_iedConnection = nullptr;
        IedClientError      m_networkStack_error = IED_ERROR_OK;

        // Section: see the class as a white box for unit tests
        FRIEND_TEST(IEC61850ClientConnectionTest, openConnection);
};

#endif  // INCLUDE_IEC61850_CLIENT_CONNECTION_H_
