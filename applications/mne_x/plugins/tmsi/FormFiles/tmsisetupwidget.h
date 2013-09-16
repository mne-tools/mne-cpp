//=============================================================================================================
/**
* @file     tmsisetupwidget.h
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     September, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Contains the declaration of the TMSISetupWidget class.
*
*/

#ifndef TMSISETUPWIDGET_H
#define TMSISETUPWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include "../ui_tmsisetup.h"


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE TMSIPlugin
//=============================================================================================================

namespace TMSIPlugin
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class TMSI;


//=============================================================================================================
/**
* DECLARE CLASS TMSISetupWidget
*
* @brief The TMSISetupWidget class provides the TMSI configuration window.
*/
class TMSISetupWidget : public QWidget
{
    Q_OBJECT
public:

    //=========================================================================================================
    /**
    * Constructs a TMSISetupWidget which is a child of parent.
    *
    * @param [in] parent pointer to parent widget; If parent is 0, the new ECGSetupWidget becomes a window. If parent is another widget, ECGSetupWidget becomes a child window inside parent. ECGSetupWidget is deleted when its parent is deleted.
    * @param [in] simulator a pointer to the corresponding ECGSimulator.
    */
    TMSISetupWidget(TMSI* simulator, QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the ECGSetupWidget.
    * All ECGSetupWidget's children are deleted first. The application exits if ECGSetupWidget is the main widget.
    */
    ~TMSISetupWidget();

    //=========================================================================================================
    /**
    * Initializes the Connector properties.
    *
    */
    void initSamplingProperties();

private:

    //=========================================================================================================
    /**
    * Sets the Sampling frequency.
    *
    */
    void setSamplingFreq(int value);

    //=========================================================================================================
    /**
    * Sets the number of channels.
    *
    */
    void setNumberOfChannels(int value);

    //=========================================================================================================
    /**
    * Sets the samples taken per block.
    *
    */
    void setSamplesPerBlock(int value);

    //=========================================================================================================
    /**
    * Shows the About Dialog
    *
    */
    void showAboutDialog();


    TMSI*           m_pTMSI;    /**< a pointer to corresponding TMSI.*/

    Ui::TMSISetupClass ui;                       /**< the user interface for the TMSISetupWidget.*/
};

} // NAMESPACE

#endif // TMSISETUPWIDGET_H
