//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2019-2026 MNE-CPP Authors
 *
 * @file     lsladaptersetup.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2019
 * @brief    Contains the implementation of the LSLAdapterSetup class.
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

void LSLAdapterSetup::onLSLScanResults(const QVector<LSLLIB::stream_info>& vStreamInfos,
                                       const LSLLIB::stream_info& currentStream)
{
    // clear UI list
    ui.listLSLStreams->clear();
    // mark m_pCurentSelectedStream as invalid (it will be deleted by the subsequent call to clear on the map).
    m_pCurrentSelectedStream = Q_NULLPTR;
    // clear mapping and create items
    m_mItemToStreamInfo.clear();
    for (LSLLIB::stream_info streamInfo : vStreamInfos) {
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
