//=============================================================================================================
/**
* @file     rerefaboutwidget.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the ReRefAboutWidget class.
*
*/

#ifndef REREFABOUTWIDGET_H
#define REREFABOUTWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../ui_rerefabout.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE ReRefPlugin
//=============================================================================================================

namespace REREFPLUGIN
{


//=============================================================================================================
/**
* DECLARE CLASS ReRefAboutWidget
*
* @brief The ReRefAboutWidget class provides the about dialog for the car.
*/
class ReRefAboutWidget : public QDialog
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
    * Constructs a ReRefAboutWidget dialog which is a child of parent.
    *
    * @param [in] parent pointer to parent widget; If parent is 0, the new carAboutWidget becomes a window. If parent is another widget, carAboutWidget becomes a child window inside parent. carAboutWidget is deleted when its parent is deleted.
    */
    ReRefAboutWidget(QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the ReRefAboutWidget.
    * All ReRefAboutWidget's children are deleted first. The application exits if carAboutWidget is the main widget.
    */
    ~ReRefAboutWidget();

private:

    Ui::ReRefAboutWidgetClass ui;		/**< Holds the user interface for the carAboutWidget.*/
};

} // NAMESPACE

#endif // REREFABOUTWIDGET_H
