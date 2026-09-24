//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     dummysetupwidget.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Definition of the DummySetupWidget class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dummysetupwidget.h"
#include "ui_dummysetup.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DUMMYTOOLBOXPLUGIN;
using namespace Ui;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DummySetupWidget::DummySetupWidget(DummyToolbox* toolbox, QWidget *parent)
: QWidget(parent)
, m_pDummyToolbox(toolbox)
{
    m_pUi = new DummySetupWidgetClass();
    m_pUi->setupUi(this);

    //Always connect GUI elemts after m_pUi->setpUi has been called
    connect(m_pUi->m_qPushButton_About, SIGNAL(released()), this, SLOT(showAboutDialog()));
}

//=============================================================================================================

DummySetupWidget::~DummySetupWidget()
{
}
