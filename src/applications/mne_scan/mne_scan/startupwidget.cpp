//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     startupwidget.cpp
 * @author   Andreas Griesshammer <ag@fieldlineinc.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Contains Definition of StartUpWidget class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "startupwidget.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QLabel>
#include <QVBoxLayout>

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNESCAN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

StartUpWidget::StartUpWidget(QWidget *parent)
: QWidget(parent)
{

    QWidget *topFiller = new QWidget;
    topFiller->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_pLabel_Info = new QLabel(tr("MNE Scan - Acquisition & Processing"), this);
    m_pLabel_Info->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    m_pLabel_Info->setAlignment(Qt::AlignCenter);

    QWidget *bottomFiller = new QWidget;
    bottomFiller->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(topFiller);
    layout->addWidget(m_pLabel_Info);
    layout->addWidget(bottomFiller);

    this->setLayout(layout);
}

//=============================================================================================================

StartUpWidget::~StartUpWidget()
{
}
