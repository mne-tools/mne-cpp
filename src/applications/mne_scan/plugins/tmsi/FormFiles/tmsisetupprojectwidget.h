//=============================================================================================================
/**
 * @file     tmsisetupprojectwidget.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the TMSISetupProjectWidget class.
 *
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
