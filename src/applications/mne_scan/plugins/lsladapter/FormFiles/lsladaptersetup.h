//=============================================================================================================
/**
 * @file     lsladaptersetup.h
 * @author   Simon Heinke <Simon.Heinke@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2019
 *
 * @section  LICENSE
 *
 * Copyright (C) 2019, Simon Heinke, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the LSLAdapterSetup class.
 *
 */

#ifndef LSLADAPTERSETUP_H
#define LSLADAPTERSETUP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ui_lsladaptersetup.h"

#include <lsl_cpp.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>

//=============================================================================================================
// DEFINE NAMESPACE LSLADAPTERPLUGIN
//=============================================================================================================

namespace LSLADAPTERPLUGIN
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * DECLARE CLASS LSLAdapterSetup
 *
 * @brief The LSLAdapterSetup class provides the LSLAdapter configuration window.
 */
class LSLAdapterSetup : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a LSLAdapterSetup which is a child of parent.
     *
     * @param[in] parent pointer to potential parent widget.
     */
    LSLAdapterSetup(int initialBlockSize, QWidget *parent = Q_NULLPTR);

    //=========================================================================================================
    /**
     * Destructor of LSLAdapterSetup: default
     */
    ~LSLAdapterSetup() = default;

public slots:
    //=========================================================================================================
    /**
     * This is called by the LSL Adapter, when it has finished scanning and filtering available LSL streams.
     *
     * @param[in] vStreamInfos A vector of available LSL streams.
     * @param[in] currentStream The current LSL stream.
     */
    void onLSLScanResults(const QVector<lsl::stream_info>& vStreamInfos, const lsl::stream_info& currentStream);

private slots:
    // auto-generated slots:
    void on_refreshAvailableStreams_released();

    void on_listLSLStreams_itemDoubleClicked(QListWidgetItem *pItem);

    void on_blockSizeEdit_editingFinished();

private:
    //=========================================================================================================
    /**
     * Helper method to update textfields.
     */
    void updateTextFields();

    QMap<QListWidgetItem*, lsl::stream_info>    m_mItemToStreamInfo;
    QListWidgetItem*                            m_pCurrentSelectedStream;

    Ui::LSLSetupWidget                          ui;

signals:
    //=========================================================================================================
    /**
     * This tells the LSL Adapter that the user wants to refresh the displayed list of available LSL streams.
     */
    void refreshAvailableStreams();

    //=========================================================================================================
    /**
     * This tells the LSL Adapter that the user has changed the stream selection.
     *
     * @param[in] stream The newly selected LSL stream, represented by stream_info object.
     */
    void streamSelectionChanged(const lsl::stream_info& stream);

    //=========================================================================================================
    /**
     * This tells the LSL Adapter that the user has changed the desired output block size
     */
    void blockSizeChanged(const int newBlockSize);
};
} // NAMESPACE

#endif // LSLADAPTERSETUP_H
