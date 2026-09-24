//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     covariancesetupwidget.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Definition of the CovarianceSetupWidget class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "covariancesetupwidget.h"

#include "../covariance.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace COVARIANCEPLUGIN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CovarianceSetupWidget::CovarianceSetupWidget(Covariance* toolbox, QWidget *parent)
: QWidget(parent)
, m_pCovariance(toolbox)
{
    ui.setupUi(this);
}

//=============================================================================================================

CovarianceSetupWidget::~CovarianceSetupWidget()
{
}
