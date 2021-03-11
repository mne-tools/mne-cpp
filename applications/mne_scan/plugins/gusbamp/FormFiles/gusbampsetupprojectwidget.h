//=============================================================================================================
/**
 * @file     gusbampsetupprojectwidget.h
 * @author   Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     March 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Viktor Klueber, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the GUSBAmpSetupProjectWidget class.
 *
 */

#ifndef GUSBAMPSETUPPROJECTWIDGET_H
#define GUSBAMPSETUPPROJECTWIDGET_H

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
class GUSBAmpSetupProjectWidget;
}

//=============================================================================================================
// DEFINE NAMESPACE GUSBAMPPLUGIN
//=============================================================================================================

namespace GUSBAMPPLUGIN
{

//=============================================================================================================
// GUSBAMPPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

class GUSBAmp;

//=============================================================================================================
/**
 * DECLARE CLASS GUSBAmpSetupProjectWidget
 *
 * @brief The GUSBAmpSetupProjectWidget class provides the GUSBAmpSetupProjectWidget configuration window.
 */
class GUSBAmpSetupProjectWidget : public QWidget
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
    explicit GUSBAmpSetupProjectWidget(GUSBAmp* pGUSBAmp,
                                       QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destructs a which is a child of parent.
     *
     */
    ~GUSBAmpSetupProjectWidget();

    //=========================================================================================================
    /**
     * Inits the GUI
     *
     */
    void initGui();

private:
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

    GUSBAmp*                        m_pGUSBAmp;         /**< a pointer to corresponding GUSBAmp.*/
    Ui::GUSBAmpSetupProjectWidget*  ui;                 /**< the user interface for the GUSBAmpSetupProjectWidget.*/
};
} // NAMESPACE

#endif // GUSBAMPSETUPPROJECTWIDGET.H
