//=============================================================================================================
/**
* @file     lsladaptersetup.cpp
* @author   Simon Heinke <simon.heinke@tu-ilmenau.de>
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2019
*
* @section  LICENSE
*
* Copyright (C) 2018, Simon Heinke and Matti Hamalainen. All rights reserved.
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

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "lsladaptersetup.h"
#include "../lsladapter.h"

#include <sstream>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QListWidgetItem>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace LSLADAPTERPLUGIN;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

LSLAdapterSetup::LSLAdapterSetup(QWidget* parent)
    : QWidget(parent)
    , m_mItemToStreamInfo()
    , ui()
{
    ui.setupUi(this);
}


//*************************************************************************************************************

void LSLAdapterSetup::onLSLScanResults(QVector<lsl::stream_info>& vStreamInfos)
{
    // clear UI list
    ui.listLSLStreams->clear();

    // clear mapping and create items
    m_mItemToStreamInfo.clear();
    for (lsl::stream_info streamInfo : vStreamInfos) {
        std::stringstream buildString;
        buildString << streamInfo.name() << ", " << streamInfo.type() << ", " << streamInfo.hostname();
        QListWidgetItem* pItem = new QListWidgetItem;
        pItem->setText(QString(buildString.str().c_str()));
        ui.listLSLStreams->addItem(pItem);

        // add to mapping
        m_mItemToStreamInfo.insert(pItem, streamInfo);
    }
}


//*************************************************************************************************************

void LSLAdapterSetup::on_connectToStream_released()
{
    if (ui.listLSLStreams->count() == 0) {
        qDebug() << "[LSLAdapterSetup] No streams in list !";
        return;
    }
    QListWidgetItem* currentItem = ui.listLSLStreams->currentItem();

    // get corresponding stream info by looking it up in the mapping
    lsl::stream_info stream = m_mItemToStreamInfo.value(currentItem);
    emit startStream(stream);
}


//*************************************************************************************************************

void LSLAdapterSetup::on_stopStreaming_released()
{
    // simply pass on to LSL Adapter
    emit stopStream();
}


//*************************************************************************************************************

void LSLAdapterSetup::on_refreshAvailableStreams_released()
{
    // simply pass on to LSL Adapter
    emit refreshAvailableStreams();
}
