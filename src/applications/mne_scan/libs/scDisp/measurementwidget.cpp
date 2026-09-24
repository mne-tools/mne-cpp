//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     measurementwidget.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     July, 2012
 * @brief    Definition of the MeasurementWidget Class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "measurementwidget.h"

#include <disp/viewers/quickcontrolview.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCDISPLIB;
using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MeasurementWidget::MeasurementWidget(QWidget* parent)
: QWidget(parent)
, m_bDisplayWidgetsInitialized(false)
{
}

//=============================================================================================================

MeasurementWidget::~MeasurementWidget()
{
}
