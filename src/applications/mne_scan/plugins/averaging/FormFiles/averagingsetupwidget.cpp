//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     averagingsetupwidget.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Definition of the AveragingSetupWidget class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averagingsetupwidget.h"

#include "../averaging.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace AVERAGINGPLUGIN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AveragingSetupWidget::AveragingSetupWidget(Averaging* toolbox, QWidget *parent)
: QWidget(parent)
, m_pAveraging(toolbox)
{
    ui.setupUi(this);
}

//=============================================================================================================

AveragingSetupWidget::~AveragingSetupWidget()
{
}
