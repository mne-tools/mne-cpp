//=============================================================================================================
/**
 * @file     ssvepbciaboutwidget.h
 * @author   Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     May, 2016
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
 * @brief    Contains the declaration of the ssvepBCIAboutWidget class.
 *
 */

#ifndef SSVEPBCIABOUTWIDGET_H
#define SSVEPBCIABOUTWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../ui_ssvepbciaboutwidget.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE SSVEPBCIPLUGIN
//=============================================================================================================

namespace SSVEPBCIPLUGIN
{


//=============================================================================================================
/**
 * DECLARE CLASS SsvepBciAboutWidget
 *
 * @brief The BCIAboutWidget class provides the about dialog for the BCI.
 */
class SsvepBciAboutWidget : public QDialog
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
    * Constructs a BCIAboutWidget dialog which is a child of parent.
    *
    * @param [in] parent pointer to parent widget; If parent is 0, the new BCIAboutWidget becomes a window. If parent is another widget, BCIAboutWidget becomes a child window inside parent. TMSIAboutWidget is deleted when its parent is deleted.
    */
    SsvepBciAboutWidget(QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destroys the BCIAboutWidget.
    * All BCIAboutWidget's children are deleted first. The application exits if BCIAboutWidget is the main widget.
    */
    ~SsvepBciAboutWidget();

private:
    Ui::BCIAboutWidgetClass ui;    /**< Holds the user interface for the BCIAboutWidgetClass.*/
};

} // NAMESPACE

#endif // SSVEPABOUTWIDGET_H
