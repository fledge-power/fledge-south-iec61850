/*
 * Fledge IEC 61850 south plugin.
 *
 * Copyright (c) 2022, RTE (https://www.rte-france.com)
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Mikael Bourhis-Cloarec
 */

#include "./wrapped_mms.h"

// Fledge headers
#include <logger.h>

// libiec61850 headers
#include <libiec61850/iec61850_common.h>


WrappedMms::~WrappedMms()
{
    if (m_mmsValue) {
        Logger::getLogger()->debug("WrappedMms: destructor 0x%x", m_mmsValue);
        MmsValue_delete(m_mmsValue);
    }
}

void WrappedMms::setMmsValue(MmsValue *mmsValue)
{
    Logger::getLogger()->debug("WrappedMms: setMmsValue 0x%x", mmsValue);
    m_mmsValue = mmsValue;
}

const MmsValue *WrappedMms::getMmsValue() const
{
    return m_mmsValue;
}

bool WrappedMms::isNull() const
{
    return (m_mmsValue == nullptr);
}
