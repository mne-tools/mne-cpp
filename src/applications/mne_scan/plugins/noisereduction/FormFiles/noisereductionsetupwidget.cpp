//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     noisereductionsetupwidget.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2016
 * @brief    Definition of the NoiseReductionSetupWidget class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "noisereductionsetupwidget.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace NOISEREDUCTIONPLUGIN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NoiseReductionSetupWidget::NoiseReductionSetupWidget(NoiseReduction* toolbox, QWidget *parent)
: QWidget(parent)
, m_pNoiseReduction(toolbox)
{
    ui.setupUi(this);
}

//=============================================================================================================

NoiseReductionSetupWidget::~NoiseReductionSetupWidget()
{
}
