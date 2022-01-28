//=============================================================================================================
/**
 * @file     neurofeedbacksetupwidget.h
 * @author   Simon Marxgut <simon.marxgut@umit-tirol.at>
 * @since    0.1.0
 * @date     November, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Simon Marxgut. All rights reserved.
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
 * @brief    Contains the declaration of the NeurofeedbackSetupWidget class.
 *
 */

#ifndef NEUROFEEDBACKSETUPWIDGET_H
#define NEUROFEEDBACKSETUPWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ui_neurofeedbacksetup.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QTabWidget>

//=============================================================================================================
// DEFINE NAMESPACE NEUROFEEDBACKPLUGIN
//=============================================================================================================

namespace NEUROFEEDBACKPLUGIN
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class Neurofeedback;


//=============================================================================================================
/**
 * DECLARE CLASS NeurofeedbackSetupWidget
 *
 * @brief The NeurofeedbackSetupWidget class provides the Neurofeedback configuration window.
 */
class NeurofeedbackSetupWidget : public QWidget
{
    Q_OBJECT

public:

    //=========================================================================================================
    /**
     * Constructs a NeurofeedbackSetupWidget which is a child of parent.
     *
     * @param [in] toolbox a pointer to the corresponding Neurofeedback.
     * @param [in] parent pointer to parent widget; If parent is 0, the new NeurofeedbackSetupWidget becomes a window. If parent is another widget, NeurofeedbackSetupWidget becomes a child window inside parent. DummySetupWidget is deleted when its parent is deleted.
     */
    explicit NeurofeedbackSetupWidget(Neurofeedback* pNeurofeedback, const QString& sSettingsPath = "", QWidget *parent = nullptr);

    //=========================================================================================================
    /**
     * Destroys the NeurofeedbackSetupWidget.
     * All NeurofeedbackSetupWidget's children are deleted first. The application exits if NeurofeedbackSetupWidget is the main widget.
     */
    ~NeurofeedbackSetupWidget();

protected:
    //=========================================================================================================
    /**
     * Saves all important settings of this view via QSettings.
     *
     * @param[in] settingsPath        the path to store the settings to.
     */
    void saveSettings(const QString& settingsPath);

    //=========================================================================================================
    /**
     * Loads and inits all important settings of this view via QSettings.
     *
     * @param[in] settingsPath        the path to load the settings from.
     */
    void loadSettings(const QString& settingsPath);

    void changeiOutput(int value);

private:
    Neurofeedback* m_pNeurofeedback;                    /**< Holds a pointer to corresponding Neurofeedback.*/

    Ui::NeurofeedbackSetupWidgetClass  ui;              /**< Holds the user interface for the NeurofeedbackSetupWidget.*/

    QString     m_sSettingsPath;
    qint32      m_iOutput;

};
} // NAMESPACE

#endif // NEUROFEEDBACKSETUPWIDGET_H
