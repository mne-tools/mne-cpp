//=============================================================================================================
/**
* @file     rapmusictoolboxaboutwidget.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     March, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Christoph Dinh. All rights reserved.
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
* @brief    Contains the declaration of the RapMusicToolboxAboutWidget class.
*
*/

#ifndef RAPMUSICTOOLBOXABOUTWIDGET_H
#define RAPMUSICTOOLBOXABOUTWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../ui_rapmusictoolboxabout.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE RapMusicToolboxPlugin
//=============================================================================================================

namespace RAPMUSICTOOLBOXPLUGIN
{


//=============================================================================================================
/**
* DECLARE CLASS RapMusicToolboxAboutWidget
*
* @brief The RapMusicToolboxAboutWidget class provides the about dialog for the RapMusicToolbox.
*/
class RapMusicToolboxAboutWidget : public QDialog
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
    * Constructs a RapMusicToolboxAboutWidget dialog which is a child of parent.
    *
    * @param [in] parent pointer to parent widget; If parent is 0, the new RapMusicToolboxAboutWidget becomes a window. If parent is another widget, DummyAboutWidget becomes a child window inside parent. DummyAboutWidget is deleted when its parent is deleted.
    */
    RapMusicToolboxAboutWidget(QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the RapMusicToolboxAboutWidget.
    * All DummyAboutWidget's children are deleted first. The application exits if RapMusicToolboxAboutWidget is the main widget.
    */
    ~RapMusicToolboxAboutWidget();

private:

    Ui::RapMusicToolboxAboutWidgetClass ui;		/**< Holds the user interface for the RapMusicToolboxAboutWidget.*/

};

} // NAMESPACE

#endif // RAPMUSICTOOLBOXABOUTWIDGET_H
