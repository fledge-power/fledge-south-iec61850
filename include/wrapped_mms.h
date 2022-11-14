#ifndef INCLUDE_WRAPPED_MMS_H_
#define INCLUDE_WRAPPED_MMS_H_

/*
 * Fledge IEC 61850 south plugin.
 *
 * Copyright (c) 2022, RTE (https://www.rte-france.com)
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Mikael Bourhis-Cloarec
 */


/** \class Mms
 *  \brief Encapsulate an MmsValue pointer
 *
 *  Encapsulate an MmsValue pointer for automatically deleting the
 *  allocated memory at the end of the life cycle of this object
 */

#include <memory>

// libiec61850 headers
#include <libiec61850/iec61850_client.h>

class WrappedMms
{
    public:
        WrappedMms() = default;
        ~WrappedMms();

        /** Disable copy constructor */
        WrappedMms(const WrappedMms &) = delete;
        /** Disable copy assignment operator */
        WrappedMms &operator = (const WrappedMms &) = delete;
        /** Disable move constructor */
        WrappedMms(WrappedMms &&) = delete;
        /** Disable move assignment operator */
        WrappedMms &operator = (WrappedMms &&) = delete;

        void setMmsValue(MmsValue *mmsValue);
        const MmsValue *getMmsValue() const;

    private:
        MmsValue *m_mmsValue = nullptr;
};

#endif  // INCLUDE_WRAPPED_MMS_H_
