//=============================================================================================================
/**
 * @file     ftbuffersetupwidget.cpp
 * @author   Gabriel B Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Gabriel B Motta. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Definition of the FtBufferSetupWidget class.
 *
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

    m_pUi->frame_BufferStatus->hide();
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
    m_pUi->frame_BufferStatus->show();
    if (stat) {
        displayBufferSettings(m_pFtBuffer->m_pFtBuffProducer->getBufferInfo());
    } else {
        displayConnectionFailed();
    }
}

//=============================================================================================================

void FtBufferSetupWidget::displayBufferSettings(BufferInfo bufferInfo)
{
    m_pUi->label_noData->hide();
    showSettings();

    m_pUi->label_numChannels->setText(QString::number(bufferInfo.iNumChannels));
    m_pUi->label_sampleFreq->setText(QString::number(bufferInfo.fSampleFreq));

    if(!bufferInfo.lHeaderChunks.empty()){
        QString extHdText = "| ";
        for(auto header : bufferInfo.lHeaderChunks){
            switch (header) {
            case HeaderChunk::FT_CHUNK_NEUROMAG_HEADER:
                extHdText += "Neuromag | ";
                break;
            case HeaderChunk::FT_CHUNK_NEUROMAG_ISOTRAK:
                extHdText += "Isotrak | ";
                break;
            default:
                break;
            }
        }
        m_pUi->label_extendedHeader->setText(extHdText);
    }
}

//=============================================================================================================

void FtBufferSetupWidget::displayConnectionFailed()
{
    m_pUi->label_noData->show();
    hideSettings();
}

//=============================================================================================================

void FtBufferSetupWidget::hideSettings()
{
    m_pUi->label_extendedHeader->hide();
    m_pUi->label_extendedHeaderTitle->hide();

    m_pUi->label_numChannels->hide();
    m_pUi->label_numChannelsTitle->hide();

    m_pUi->label_sampleFreq->hide();
    m_pUi->label_sampleFreqTitle->hide();
}

//=============================================================================================================

void FtBufferSetupWidget::showSettings()
{
    m_pUi->label_extendedHeader->show();
    m_pUi->label_extendedHeaderTitle->show();

    m_pUi->label_numChannels->show();
    m_pUi->label_numChannelsTitle->show();

    m_pUi->label_sampleFreq->show();
    m_pUi->label_sampleFreqTitle->show();
}
