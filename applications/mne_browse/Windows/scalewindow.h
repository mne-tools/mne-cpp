//=============================================================================================================
/**
 * @file     scalewindow.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     October, 2014
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
 * @brief    Contains the declaration of the ScaleWindow class.
 *
 */

#ifndef SCALEWINDOW_H
#define SCALEWINDOW_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ui_scalewindow.h"
#include <fiff/fiff.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDockWidget>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNEBROWSE
//=============================================================================================================

namespace MNEBROWSE
{


//*************************************************************************************************************
//=============================================================================================================
// DEFINE FORWARD DECLARATIONS
//=============================================================================================================

/**
 * DECLARE CLASS ScaleWindow
 *
 * @brief The ScaleWindow class provides the scale window.
 */
class ScaleWindow : public QDockWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a ScaleWindow dialog which is a child of parent.
     *
     * @param [in] parent pointer to parent widget; If parent is 0, the new ScaleWindow becomes a window. If parent is another widget, ScaleWindow becomes a child window inside parent. ScaleWindow is deleted when its parent is deleted.
     */
    ScaleWindow(QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the ScaleWindow.
     * All ScaleWindow's children are deleted first. The application exits if ScaleWindow is the main widget.
     */
    ~ScaleWindow();

    //=========================================================================================================
    /**
     * Initialises this window.
     */
    void init();

    //=========================================================================================================
    /**
     * hideSpinBoxes hides all spin boxes and labels which are not present in the current fiff file.
     *
     * @param [in] currentFiffInfo the fiff info file used to hide the spin boxes for not loaded channel types
     */
    void hideSpinBoxes(FiffInfo::SPtr& pCurrentFiffInfo);

    //=========================================================================================================
    /**
     * scaleAllChannels scales all channels by scaleValue.
     *
     * @param [in] scaleValue the scaling value used to scale the channels
     */
    void scaleAllChannels(double scaleValue);

signals:
    //=========================================================================================================
    /**
     * scalingChannelValueChanged is emmited whenever a connected data spin box value changed
     *
     * @param [in] QMap<QString,double> map with all channel types and their current scaling value
     */
    void scalingChannelValueChanged(QMap<QString,double>);

    //=========================================================================================================
    /**
     * scalingViewValueChanged is emmited whenever a connected view spin box value changed
     *
     * @param [in] int current scaling value for the views
     */
    void scalingViewValueChanged(int);

private:
    Ui::ScaleWindow *ui;            /**< Pointer to the qt designer generated ui class.*/

    //=========================================================================================================
    /**
     * Returns the current scaling map which generated out of the current spin box values
     *
     * @return the generated scale value map for each channel type
     */
    QMap<QString,double> genereateScalingMap();

    //=========================================================================================================
    /**
     * scaleChannelValueChanged is called whenever a data spin box value changed.
     */
    void scaleChannelValueChanged();

    //=========================================================================================================
    /**
     * scaleViewValueChanged is called whenever a view spin box value changed.
     */
    void scaleViewValueChanged();

};

} // NAMESPACE MNEBROWSE

#endif // SCALEWINDOW_H
