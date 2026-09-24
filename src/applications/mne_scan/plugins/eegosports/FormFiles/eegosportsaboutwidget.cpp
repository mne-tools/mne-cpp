//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     eegosportsaboutwidget.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     July, 2014
 * @brief    Contains the implementation of the EEGoSportsAboutWidget class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eegosportsaboutwidget.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace EEGOSPORTSPLUGIN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EEGoSportsAboutWidget::EEGoSportsAboutWidget(QWidget *parent)
: QDialog(parent)
{
    m_ui.setupUi(this);
}

//=============================================================================================================

EEGoSportsAboutWidget::~EEGoSportsAboutWidget()
{
}
