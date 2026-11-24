//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     iconnector.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Out-of-line members of @ref RTSERVER::IConnector: construction and the callback channels.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "iconnector.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTSERVER;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

IConnector::IConnector() = default;

//=============================================================================================================

IConnector::~IConnector() = default;

//=============================================================================================================

void IConnector::setMeasInfoCallback(MeasInfoCallback callback)
{
    const std::scoped_lock lock(m_callbackMutex);
    m_measInfoCallback = std::move(callback);
}

//=============================================================================================================

void IConnector::setRawBufferCallback(RawBufferCallback callback)
{
    const std::scoped_lock lock(m_callbackMutex);
    m_rawBufferCallback = std::move(callback);
}

//=============================================================================================================

void IConnector::remitMeasInfo(std::int32_t ID, const mne::FiffInfo& p_info)
{
    MeasInfoCallback callback;
    {
        const std::scoped_lock lock(m_callbackMutex);
        callback = m_measInfoCallback;
    }
    if (callback) {
        callback(ID, p_info);
    }
}

//=============================================================================================================

void IConnector::remitRawBuffer(std::shared_ptr<const Eigen::MatrixXf> p_buffer)
{
    RawBufferCallback callback;
    {
        const std::scoped_lock lock(m_callbackMutex);
        callback = m_rawBufferCallback;
    }
    if (callback) {
        callback(std::move(p_buffer));
    }
}
