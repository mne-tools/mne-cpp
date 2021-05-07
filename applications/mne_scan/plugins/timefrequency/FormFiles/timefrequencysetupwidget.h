//=============================================================================================================
/**
 * @file     timefrequencysetupwidget.h
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.9
 * @date     April, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Gabriel Motta. All rights reserved.
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
 * @brief    Contains the declaration of the TimeFrequencySetupWidget class.
 *
 */

#ifndef TIMEFREQUENCYSETUPWIDGET_H
#define TIMEFREQUENCYSETUPWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ui_timefrequencysetup.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>

//=============================================================================================================
// DEFINE NAMESPACE TIMEFREQUENCYPLUGIN
//=============================================================================================================

namespace TIMEFREQUENCYPLUGIN
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class TimeFrequency;

//=============================================================================================================
/**
 * DECLARE CLASS TimeFrequencySetupWidget
 *
 * @brief The TimeFrequencySetupWidget class provides the TimeFrequencyToolbox configuration window.
 */
class TimeFrequencySetupWidget : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
     * Constructs a TimeFrequencySetupWidget which is a child of parent.
     *
     * @param [in] toolbox a pointer to the corresponding TimeFrequency toolbox.
     * @param [in] parent pointer to parent widget; If parent is 0, the new TimeFrequencySetupWidget becomes a window. If parent is another widget, TimeFrequencySetupWidget becomes a child window inside parent. TimeFrequencySetupWidget is deleted when its parent is deleted.
     */
    TimeFrequencySetupWidget(TimeFrequency* toolbox, QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destroys the TimeFrequencySetupWidget.
     * All TimeFrequencySetupWidget's children are deleted first. The application exits if TimeFrequencySetupWidget is the main widget.
     */
    ~TimeFrequencySetupWidget();

private:

    TimeFrequency* m_pTimeFrequency;        /**< Holds a pointer to corresponding TimeFrequency.*/

    Ui::TimeFrequencySetupWidgetClass ui;   /**< Holds the user interface for the TimeFrequencySetupWidget.*/
};
} // NAMESPACE

#endif // TIMEFREQUENCYSETUPWIDGET_H