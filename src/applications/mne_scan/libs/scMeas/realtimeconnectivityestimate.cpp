//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     realtimeconnectivityestimate.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     October, 2016
 * @brief    Definition of the RealTimeConnectivityEstimate class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimeconnectivityestimate.h"

#include <connectivity/network/network.h>

#include <mne/mne_forward_solution.h>

#include <fs/fs_surfaceset.h>
#include <fs/fs_annotationset.h>

#include <fiff/fiff_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCMEASLIB;
using namespace CONNECTIVITYLIB;
using namespace MNELIB;
using namespace FSLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeConnectivityEstimate::RealTimeConnectivityEstimate(QObject *parent)
: Measurement(QMetaType::fromName("RealTimeConnectivityEstimate::SPtr").id(), parent)
, m_pAnnotSet(FsAnnotationSet::SPtr(new FsAnnotationSet))
, m_pSurfSet(FsSurfaceSet::SPtr(new FsSurfaceSet))
, m_pFwdSolution(MNEForwardSolution::SPtr(new MNEForwardSolution))
, m_pNetwork(Network::SPtr(new Network))
, m_bInitialized(false)
{
}

//=============================================================================================================

RealTimeConnectivityEstimate::~RealTimeConnectivityEstimate()
{
}

//=============================================================================================================

QSharedPointer<Network> &RealTimeConnectivityEstimate::getValue()
{
    QMutexLocker locker(&m_qMutex);
    return m_pNetwork;
}

//=============================================================================================================

void RealTimeConnectivityEstimate::setValue(const Network& v)
{
    m_qMutex.lock();

    //Store
     *m_pNetwork = v;

    m_bInitialized = true;

    m_qMutex.unlock();

    emit notify();
}

