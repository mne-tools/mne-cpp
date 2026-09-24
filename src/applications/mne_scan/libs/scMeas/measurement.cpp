//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     measurement.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     August, 2013
 * @brief    Definition of the Measurement base class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "measurement.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCMEASLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Measurement::Measurement(int type,
                         QObject *parent)
: QObject(parent)
, m_iMetaTypeId(type)
, m_bVisibility(true)
{
//    qWarning() << "QMetaType" << type;
}

//=============================================================================================================

Measurement::~Measurement()
{
}
