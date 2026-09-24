//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     realtimespectrum.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     July, 2014
 * @brief    Definition of the RealTimeSpectrum class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "realtimespectrum.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCMEASLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RealTimeSpectrum::RealTimeSpectrum(QObject *parent)
: Measurement(QMetaType::fromName("RealTimeSpectrum::SPtr").id(), parent)
, m_bIsInit(false)
, m_bContainsValues(false)
, m_xScaleType(0)
{
}

//=============================================================================================================

RealTimeSpectrum::~RealTimeSpectrum()
{
}

//=============================================================================================================

void RealTimeSpectrum::initFromFiffInfo(FiffInfo::SPtr &p_pFiffInfo)
{
    m_pFiffInfo = p_pFiffInfo;

    m_bIsInit = true;
}

//=============================================================================================================

void RealTimeSpectrum::initScaleType(qint8 ScaleType)
{
    m_xScaleType = ScaleType;
}

//=============================================================================================================

MatrixXd RealTimeSpectrum::getValue() const
{
    return m_matValue;
}

//=============================================================================================================

void RealTimeSpectrum::setValue(MatrixXd& v)
{
    //Store
    m_matValue = v;
    emit notify();

    if(!m_bContainsValues)
        m_bContainsValues = true;
}

