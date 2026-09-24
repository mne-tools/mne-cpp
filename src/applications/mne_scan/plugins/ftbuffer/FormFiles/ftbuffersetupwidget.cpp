//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     ftbuffersetupwidget.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     January, 2020
 * @brief    Definition of the FtBufferSetupWidget class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ftbuffersetupwidget.h"
#include "ui_ftbuffersetup.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FTBUFFERPLUGIN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FtBufferSetupWidget::FtBufferSetupWidget(FtBuffer* toolbox,
                                         const QString& sSettingsPath,
                                         QWidget *parent)
: QWidget(parent)
, m_pFtBuffer(toolbox)
, m_sSettingsPath(sSettingsPath)
, m_pUi(new Ui::FtBufferSetupUi)
{
    m_pUi->setupUi(this);

    this->m_pUi->m_lineEditIP->setText(toolbox->m_pFtBuffProducer->m_pFtConnector->getAddr());

    loadSettings();

    //Always connect GUI elemts after m_pUi->setpUi has been called
    connect(m_pUi->m_qPushButton_Connect, &QPushButton::released,
            this, &FtBufferSetupWidget::pressedConnect); // Connect/Disconnect button

    connect(this, &FtBufferSetupWidget::connectAtAddr,
            m_pFtBuffer->m_pFtBuffProducer.data(), &FtBuffProducer::connectToBuffer);
    connect(m_pFtBuffer->m_pFtBuffProducer.data(), &FtBuffProducer::connecStatus,
            this, &FtBufferSetupWidget::isConnected);

    connect(m_pUi->m_lineEditIP, &QLineEdit::textChanged,
            toolbox, &FtBuffer::setBufferAddress);
    connect(m_pUi->m_spinBoxPort, QOverload<int>::of(&QSpinBox::valueChanged),
            toolbox, &FtBuffer::setBufferPort);
    toolbox->setBufferAddress(m_pUi->m_lineEditIP->text());
    toolbox->setBufferPort(m_pUi->m_spinBoxPort->value());
}

//=============================================================================================================

FtBufferSetupWidget::~FtBufferSetupWidget()
{
    saveSettings();
}

//=============================================================================================================

void FtBufferSetupWidget::saveSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    // Save Settings
    QSettings settings("MNECPP");

    settings.setValue(m_sSettingsPath + QString("/IP"), m_pUi->m_lineEditIP->text());
}

//=============================================================================================================

void FtBufferSetupWidget::loadSettings()
{
    if(m_sSettingsPath.isEmpty()) {
        return;
    }

    // Load Settings
    QSettings settings("MNECPP");

    m_pUi->m_lineEditIP->setText(settings.value(m_sSettingsPath + QString("/IP"), "127.0.0.1").toString());
}

//=============================================================================================================

void FtBufferSetupWidget::pressedConnect()
{
    emit connectAtAddr(m_pUi->m_lineEditIP->text(),
                       m_pUi->m_spinBoxPort->value());
}

//=============================================================================================================

void FtBufferSetupWidget::isConnected(bool stat)
{
    if (stat) {
        m_pUi->m_qPushButton_Connect->setText("Set");
    } else {
        qWarning() << "[FtBufferSetupWidget::isConnected] Unable to find relevant fiff info.";

        QMessageBox msgBox;
        msgBox.setText("Unable to find relevant fiff info. Is there header data in the buffer?");
        msgBox.exec();
    }
}
