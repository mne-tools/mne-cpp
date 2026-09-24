//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     tmsisetupprojectwidget.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     July 2014
 * @brief    Contains the declaration of the TMSISetupProjectWidget class.
 */

#ifndef TMSISETUPPROJECTWIDGET_H
#define TMSISETUPPROJECTWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>

namespace Ui {
class TMSISetupProjectWidget;
}

//=============================================================================================================
// DEFINE NAMESPACE TMSIPLUGIN
//=============================================================================================================

namespace TMSIPLUGIN
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class TMSI;

//=============================================================================================================
/**
 * DECLARE CLASS TMSISetupProjectWidget
 *
 * @brief The TMSISetupProjectWidget class provides the TMSISetupProjectWidget configuration window.
 */
class TMSISetupProjectWidget : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a TMSISetupProjectWidget which is a child of parent.
     *
     * @param[in] parent pointer to parent widget; If parent is 0, the new TMSISetupProjectWidget becomes a window. If parent is another widget, TMSISetupWidget becomes a child window inside parent. TMSISetupWidget is deleted when its parent is deleted.
     * @param[in] pTMSI a pointer to the corresponding ECGSimulator.
     */
    explicit TMSISetupProjectWidget(TMSI* pTMSI, QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destructs a TMSISetupProjectWidget which is a child of parent.
     *
     */
    ~TMSISetupProjectWidget();

    //=========================================================================================================
    /**
     * Inits the GUI
     *
     */
    void initGui();

private:
    TMSI*                           m_pTMSI;        /**< a pointer to corresponding TMSI.*/

    Ui::TMSISetupProjectWidget*     ui;             /**< the user interface for the TMSISetupWidget.*/

    //=========================================================================================================
    /**
     * Sets the dir where the eeg cap file is located
     *
     */
    void changeCap();

    //=========================================================================================================
    /**
     * Changes the EEG cap and file path variables in the EEGoSports class
     *
     */
    void changeQLineEdits();
};
} // NAMESPACE

#endif // TMSISETUPPROJECTWIDGET_H
