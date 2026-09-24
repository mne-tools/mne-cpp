//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     writetofilesetupwidget.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2020
 * @brief    Definition of the WriteToFileSetupWidget class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "writetofilesetupwidget.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace WRITETOFILEPLUGIN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

WriteToFileSetupWidget::WriteToFileSetupWidget(WriteToFile* toolbox, QWidget *parent)
: QWidget(parent)
, m_pWriteToFile(toolbox)
{
    ui.setupUi(this);

    ui.checkBox->setChecked(toolbox->isContinuous());

    connect(ui.checkBox, &QCheckBox::checkStateChanged,
            m_pWriteToFile, &WriteToFile::setContinuous);
    m_pWriteToFile->setContinuous(ui.checkBox->checkState());
}

//=============================================================================================================

WriteToFileSetupWidget::~WriteToFileSetupWidget()
{
}

