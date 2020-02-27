//=============================================================================================================
/**
* @file     ftbufferaboutwidget.h
* @author   Gabriel B. Motta <gbmotta@mgh.harvard.edu>
* @version  dev
* @date     January, 2020
*
* @section  LICENSE
*
* Copyright (C) 2020, Gabriel B. Motta. All rights reserved.
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
* @brief    Contains the declaration of the FtBufferAboutWidget class.
*
*/

#ifndef FTBUFFERABOUTWIDGET_H
#define FTBUFFERABOUTWIDGET_H


//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../ui_ftbufferabout.h"


//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>

//=============================================================================================================
// DEFINE NAMESPACE FtBufferToolboxPlugin
//=============================================================================================================

namespace FTBUFFERPLUGIN
{

//=============================================================================================================
/**
 * DECLARE CLASS FtBufferAboutWidget
 *
 * @brief The FtBufferAboutWidget class provides the about dialog for the FtBufferToolbox.
 */
class FtBufferAboutWidget : public QDialog
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
     * Constructs a FtBufferAboutWidget dialog which is a child of parent.
     *
     * @param [in] parent pointer to parent widget; If parent is 0, the new FtBufferAboutWidget becomes a window. If parent is another widget, FtBufferAboutWidget becomes a child window inside parent. FtBufferAboutWidget is deleted when its parent is deleted.
     */
    FtBufferAboutWidget(QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the FtBufferAboutWidget.
     * All FtBufferAboutWidget's children are deleted first. The application exits if FtBufferAboutWidget is the main widget.
     */
    ~FtBufferAboutWidget();

private:

    Ui::FtBufferAboutWidgetClass ui;		/**< Holds the user interface for the FtBufferAboutWidget.*/

};

} // NAMESPACE

#endif // FTBUFFERABOUTWIDGET_H
