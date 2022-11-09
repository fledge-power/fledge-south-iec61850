#ifndef INCLUDE_IEC61850_CLIENT_CONNECTION_INTERFACE_H_
#define INCLUDE_IEC61850_CLIENT_CONNECTION_INTERFACE_H_

/*
 * Fledge IEC 61850 south plugin.
 *
 * Copyright (c) 2022, RTE (https://www.rte-france.com)
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Mikael Bourhis-Cloarec
 */

// libiec61850 headers
#include <libiec61850/iec61850_client.h>

// local library
#include "./wrapped_mms.h"

class IEC61850ClientConnectionInterface
{
    public :

        virtual ~IEC61850ClientConnectionInterface() = default;

        virtual bool isConnected() = 0;
        virtual bool isNoError() const = 0;
        virtual void logError() const = 0;

        virtual std::shared_ptr<WrappedMms> readMms(const std::string &daPath,
                                                    const std::string &fcName) = 0;

};
#endif  // INCLUDE_IEC61850_CLIENT_CONNECTION_INTERFACE_H_
