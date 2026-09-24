//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2019-2026 MNE-CPP Authors
 *
 * @file     lsladaptersetup.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2019
 * @brief    Contains the declaration of the LSLAdapterSetup class.
 */

#ifndef LSLADAPTERSETUP_H
#define LSLADAPTERSETUP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ui_lsladaptersetup.h"

#include <lsl/lsl.h>

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
    void onLSLScanResults(const QVector<LSLLIB::stream_info>& vStreamInfos, const LSLLIB::stream_info& currentStream);

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

    QMap<QListWidgetItem*, LSLLIB::stream_info>    m_mItemToStreamInfo;
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
    void streamSelectionChanged(const LSLLIB::stream_info& stream);

    //=========================================================================================================
    /**
     * This tells the LSL Adapter that the user has changed the desired output block size
     */
    void blockSizeChanged(const int newBlockSize);
};
} // NAMESPACE

#endif // LSLADAPTERSETUP_H
