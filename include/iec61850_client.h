#ifndef INCLUDE_IEC61850_CLIENT_H_
#define INCLUDE_IEC61850_CLIENT_H_

/*
 * Fledge IEC 61850 south plugin.
 *
 * Copyright (c) 2020, RTE (https://www.rte-france.com)
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Estelle Chigot, Lucas Barret
 */

#include <memory>
#include <thread>  // NOLINT
#include <mutex>   // NOLINT
#include <atomic>

// Fledge headers
#include <logger.h>
#include <reading.h>


// local library
#include "./iec61850_client_config.h"
#include "./iec61850_client_connection_interface.h"

// For white box unit tests
#include <gtest/gtest_prod.h>

class IEC61850;
class WrappedMms;

/** \class IEC61850Client
 *  \brief Read from and write to a IED
 *
 *  Handle IEC61850 data objects
 *  for reading data
 *  or sending order and operation
 */
class IEC61850Client
{
    public :

        explicit IEC61850Client(IEC61850 *iec61850,
                                const ServerConnectionParameters &connectionParam,
                                const ExchangedData &exchangedData);

        ~IEC61850Client();

        /** Disable default constructor */
        IEC61850Client() = delete;

        /** Disable copy constructor */
        IEC61850Client(const IEC61850Client &) = delete;
        /** Disable copy assignment operator */
        IEC61850Client &operator = (const IEC61850Client &) = delete;
        /** Disable move constructor */
        IEC61850Client(IEC61850Client &&) = delete;
        /** Disable move assignment operator */
        IEC61850Client &operator = (IEC61850Client &&) = delete;

        /**
         * Open the connection with the IED
         */
        void start();

        /**
         * Close the connection with the IED
         */
        void stop();

    private:
        std::string m_clientId;

        // Section: Configuration
        const ServerConnectionParameters &m_connectionParam;
        ExchangedData m_exchangedData;

        // Section: Data formatting for the plugin
        IEC61850 *m_iec61850;

        /**
         * Create the Datapoint object that will be ingest by Fledge
         * (dynamic allocation, deallocation by Fledge core).
         * Reentrant function, thread safe
         */
        template <typename T>
        static Datapoint *createDatapoint(const std::string &dataName,
                                          T primitiveTypeValue);

        /**
         * Convert the MMS into Datapoint
         * by extracting the MMS content and creating a new Datapoint
         * Reentrant function, thread safe
         */
        static Datapoint *convertMmsToDatapoint(std::shared_ptr<WrappedMms> wrappedMms);

        /**
         * Send a datapoint to Fledge core
         * Reentrant function, thread safe
         */
        void sendData(Datapoint *datapoint);

        void readAndExportMms();

        // Section: Client initialization with connection creation
        void launch();
        void initializeConnection();
        void createConnection();
        void destroyConnection();
        std::thread m_backgroundLaunchThread;
        std::atomic<bool> m_stopOrder{false};
        std::unique_ptr<IEC61850ClientConnectionInterface> m_connection;

        // Section: For demo only
        void startDemo();
        void stopDemo();
        void readMmsLoop();
        std::atomic<bool> m_isDemoLoopActivated{false};
        std::thread m_demoLoopThread;

        // Section: see the class as a white box for unit tests
        FRIEND_TEST(IEC61850ClientTest, createOneConnection);
        FRIEND_TEST(IEC61850ClientTest, reuseCreatedConnection);
        FRIEND_TEST(IEC61850ClientTest, destroyConnection);
        FRIEND_TEST(IEC61850ClientTest, destroyNullConnection);
        FRIEND_TEST(IEC61850ClientTest, injectMockConnection);
        FRIEND_TEST(IEC61850ClientTest, initializeConnectionInOneTry);
        FRIEND_TEST(IEC61850ClientTest, initializeConnectionFailed);
        FRIEND_TEST(IEC61850ClientTest, startAndStop);
};

#endif  // INCLUDE_IEC61850_CLIENT_H_
