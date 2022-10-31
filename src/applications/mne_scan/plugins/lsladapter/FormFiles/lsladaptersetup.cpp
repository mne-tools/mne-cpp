//=============================================================================================================
/**
 * @file     lsladaptersetup.cpp
 * @author   Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2019
 *
 * @section  LICENSE
 *
 * Copyright (C) 2019, Simon Heinke. All rights reserved.
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
 * @brief    Contains the implementation of the LSLAdapterSetup class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "lsladaptersetup.h"
#include "../lsladapter.h"

#include <sstream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QListWidgetItem>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace LSLADAPTERPLUGIN;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

LSLAdapterSetup::LSLAdapterSetup(int initialBlockSize, QWidget* parent)
: QWidget(parent)
, m_mItemToStreamInfo()
, m_pCurrentSelectedStream(Q_NULLPTR)
, ui()
{
    ui.setupUi(this);
    // copy start value for block size into UI:
    ui.blockSizeEdit->setText(QString::number(initialBlockSize));
}

//=============================================================================================================

void LSLAdapterSetup::onLSLScanResults(const QVector<lsl::stream_info>& vStreamInfos,
                                       const lsl::stream_info& currentStream)
{
    // clear UI list
    ui.listLSLStreams->clear();
    // mark m_pCurentSelectedStream as invalid (it will be deleted by the subsequent call to clear on the map).
    m_pCurrentSelectedStream = Q_NULLPTR;
    // clear mapping and create items
    m_mItemToStreamInfo.clear();
    for (lsl::stream_info streamInfo : vStreamInfos) {
        std::stringstream buildString;
        buildString << streamInfo.name() << ", " << streamInfo.type() << ", " << streamInfo.hostname();
        QListWidgetItem* pItem = new QListWidgetItem;
        // select the current stream
        if(currentStream.uid() == streamInfo.uid()) {
            pItem->setSelected(true);
            m_pCurrentSelectedStream = pItem;
        } else {
            pItem->setSelected(false);
        }
        pItem->setText(QString(buildString.str().c_str()));

        ui.listLSLStreams->addItem(pItem);

        // add to mapping
        m_mItemToStreamInfo.insert(pItem, streamInfo);
    }

    updateTextFields();
}

//=============================================================================================================

void LSLAdapterSetup::on_refreshAvailableStreams_released()
{
    // simply pass on to LSL Adapter
    emit refreshAvailableStreams();
}

//=============================================================================================================

void LSLAdapterSetup::on_listLSLStreams_itemDoubleClicked(QListWidgetItem *pItem)
{
    m_pCurrentSelectedStream = pItem;

    updateTextFields();

    // tell adapter:
    if(m_pCurrentSelectedStream && m_mItemToStreamInfo.contains(m_pCurrentSelectedStream)) {
        emit streamSelectionChanged(m_mItemToStreamInfo.value(m_pCurrentSelectedStream));
    }
    else {
        // this should not happen
        qDebug() << "[LSLAdapterSetup] CRITICAL: Major inconsistency in UI!";
    }
}

//=============================================================================================================

void LSLAdapterSetup::updateTextFields()
{
    // current stream label:
    if(m_pCurrentSelectedStream) {
        ui.currentStreamDescription->setText(m_pCurrentSelectedStream->text());
    } else {
        ui.currentStreamDescription->setText(QString("None"));
    }
    ui.currentStreamDescription->setStyleSheet("font: bold");
}

//=============================================================================================================

void LSLAdapterSetup::on_blockSizeEdit_editingFinished()
{
    QString sInput = ui.blockSizeEdit->text();
    int iBlockSize = sInput.toInt();
    if(iBlockSize <= 1) {
        qDebug() << "[LSLAdapterSetup: blockSizeEdit: Not a valid block size: " << sInput;
    }
    else {
        emit blockSizeChanged(iBlockSize);
    }
}
