//=============================================================================================================
/**
 * @file     tmsimanualannotationwidget.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     March, 2014
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
 * @brief    Contains the declaration of the TMSIManualAnnotation class.
 *
 */

#ifndef TMSIMANUALANNOTATIONWIDGET_H
#define TMSIMANUALANNOTATIONWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include "ui_tmsimanualannotation.h"

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
 * DECLARE CLASS TMSIManualAnnotationWidget
 *
 * @brief The TMSIManualAnnotationWidget class provides a widget/window for manually annotating the trigger, which the subject pressed during the session.
 */
class TMSIManualAnnotationWidget : public QWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a TMSIManualAnnotationWidget which is a child of parent.
     *
     * @param [in] parent pointer to parent widget; If parent is 0, the new TMSIManualAnnotationWidget becomes a window. If parent is another widget, TMSIManualAnnotationWidget becomes a child window inside parent. TMSIManualAnnotationWidget is deleted when its parent is deleted.
     * @param [in] pTMSI a pointer to the corresponding ECGSimulator.
     */
    TMSIManualAnnotationWidget(TMSI* pTMSI, QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the TMSIManualAnnotationWidget.
     * All TMSIManualAnnotationWidget's children are deleted first. The application exits if TMSIManualAnnotationWidget is the main widget.
     */
    ~TMSIManualAnnotationWidget();

    //=========================================================================================================
    /**
     * Initializes the Connector's GUI properties.
     *
     */
    void initGui();

private:
    TMSI*           m_pTMSI;                /**< a pointer to corresponding TMSI.*/

    Ui::TMSIManualAnnotationWidget ui;      /**< the user interface for the TMSIManualAnnotationWidget.*/

    virtual void  keyPressEvent(QKeyEvent *event);
};
} // NAMESPACE

#endif // TMSIMANUALANNOTATIONWIDGET_H
