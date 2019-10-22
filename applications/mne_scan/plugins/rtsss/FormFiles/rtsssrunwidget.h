//=============================================================================================================
/**
* @file     rtsssrunwidget.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the RtSssRunWidget class.
*
*/

#ifndef RTSSSRUNWIDGET_H
#define RTSSSRUNWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../ui_rtsssrun.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE RTSSSPLUGIN
//=============================================================================================================

namespace RTSSSPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class RtSss;


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================


//=============================================================================================================
/**
* DECLARE CLASS SourceLabRunWidget
*
* @brief The SourceLabRunWidget class provides the SourceLab configuration window for the run mode.
*/
class RtSssRunWidget : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
    * Constructs a RtSssRunWidget which is a child of parent.
    *
    * @param [in] toolbox   a pointer to the corresponding RtSss.
    * @param [in] parent    pointer to parent widget; If parent is 0, the new RtSssRunWidget becomes a window. If parent is another widget, DummyRunWidget becomes a child window inside parent. RtSssRunWidget is deleted when its parent is deleted.
    */
    RtSssRunWidget(RtSss* toolbox, QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the RtSssRunWidget.
    * All RtSssRunWidget's children are deleted first. The application exits if RtSssRunWidget is the main widget.
    */
    ~RtSssRunWidget();

    //=========================================================================================================
    /**
    * Writes to SourceLab run log
    *
    * @param[in] p_sLogMsg     status message to append
    */
    void writeToLog(QString p_sLogMsg);

private slots:
    //=========================================================================================================
    /**
    * Shows the About Dialog
    *
    */
    void showAboutDialog();

private:

    RtSss*    m_pRtSss;     /**< Holds a pointer to corresponding RtSss.*/

    Ui::RtSssRunWidgetClass ui; /**< Holds the user interface for the RtSssRunWidget.*/
};

} // NAMESPACE

#endif // RTSSSRUNWIDGET_H
