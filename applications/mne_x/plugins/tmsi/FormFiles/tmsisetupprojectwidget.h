//=============================================================================================================
/**
* @file     tmsisetupprojectwidget.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the TMSISetupProjectWidget class.
*
*/

#ifndef TMSISETUPPROJECTWIDGET_H
#define TMSISETUPPROJECTWIDGET_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>

namespace Ui {
class TMSISetupProjectWidget;
}

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
* DECLARE CLASS TMSISetupProjectWidget
*
* @brief The TMSISetupProjectWidget class provides the TMSISetupProjectWidget configuration window.
*/
class TMSISetupProjectWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TMSISetupProjectWidget(TMSI* pTMSI, QWidget *parent = 0);
    ~TMSISetupProjectWidget();

private:
    TMSI*                           m_pTMSI;        /**< a pointer to corresponding TMSI.*/

    Ui::TMSISetupProjectWidget*     ui;             /**< the user interface for the TMSISetupWidget.*/

    //=========================================================================================================
    /**
    * Sets the dir where the output file is saved
    *
    */
    void changeOutputFileDir();

    //=========================================================================================================
    /**
    * Sets the dir where the output file is saved
    *
    */
    void setOutputTextField();

    //=========================================================================================================
    /**
    * Sets the dir where the eeg hat file is located
    *
    */
    void changeHatDir();
};

} // NAMESPACE

#endif // TMSISETUPPROJECTWIDGET_H
