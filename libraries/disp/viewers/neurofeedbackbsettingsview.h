//=============================================================================================================
/**
 * @file     neurofeedbackbsettingsview.h
 * @author   Simon Marxgut <simon.marxgut@umit-tirol.at>
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
 * @brief    Declaration of the NeurofeedbackBSettingsView Class.
 *
 */

#ifndef NEUROFEEDBACKBSETTINGSVIEW_H
#define NEUROFEEDBACKBSETTINGSVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"
#include "abstractview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QPointer>
#include <QLineEdit>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QRadioButton>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QSlider;
class QDoubleSpinBox;


namespace Ui {
class NeurofeedbackBSettingsViewWidget;
}

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

//=============================================================================================================
// DISPLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * DECLARE CLASS NeurofeedbackBSettingsView
 */
class DISPSHARED_EXPORT NeurofeedbackBSettingsView : public QWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<NeurofeedbackBSettingsView> SPtr;              /**< Shared pointer type for NeurofeedbackBSettingsView. */
    typedef QSharedPointer<const NeurofeedbackBSettingsView> ConstSPtr;   /**< Const shared pointer type for NeurofeedbackBSettingsView. */

    //=========================================================================================================
    /**
     * Constructs a NeurofeedbackBSettingsView which is a child of parent.
     *
     * @param [in] parent    parent of widget
     */
    NeurofeedbackBSettingsView(const QString& sSettingsPath = "",
                          int ibMax = 25,
                          int ibMin = 0,
                          QWidget *parent = 0,
                          Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================

    ~NeurofeedbackBSettingsView();

    //=========================================================================================================
    /**
     * Emits all settings once.
     *
     */
    void emitSignals();


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

    //=========================================================================================================
    /**
     * Slot is called when Load Button for Background is clicked
     */
    void clickedLoadBackground();

    //=========================================================================================================
    /**
     * Slot is called when Load Button for Objectimage is clicked
     */
    void clickedLoadObject();

    //=========================================================================================================
    /**
     * Slot is called when value in the Spinbox for maximum is changed
     *
     * @param[in] value        maximum of frequency.
     */
    void onUpdateSpinBoxMax(int value);

    //=========================================================================================================
    /**
     * Slot is called when value in the Spinbox for minimum is changed
     *
     * @param[in] value        minimum of frequency.
     */
    void onUpdateSpinBoxMin(int value);

    //=========================================================================================================
    /**
     * Slot is called when standard Button for maximum is clicked
     */
    void clickedStdMax();

    //=========================================================================================================
    /**
     * Slot is called when standard Button for minimum is clicked
     */
    void clickedStdMin();

    //=========================================================================================================
    /**
     * Slot is called when value in the Spinbox for the selected Channel is changed
     *
     * @param[in] value        selected channel.
     */
    void onUpdateSpinBoxChannel(int value);

    //=========================================================================================================
    /**
     * Slot is called when Reset Settings Button is clicked
     */
    void clickedResetSettings();

    //=========================================================================================================
    /**
     * Update all Settings on the NeurofeedbackBSettingsView Display.
     */
    void updateDisplay();

    //=========================================================================================================
    /**
     * Update the Status Label for the Background.
     */
    void updateStatusBackground();

    //=========================================================================================================
    /**
     * Update the Status Label for the Object.
     */
    void updateStatusObject();



    Ui::NeurofeedbackBSettingsViewWidget *m_pui;

    QString     m_sSettingsPath;
    QString     m_sDirBackground;
    QString     m_sDirObject;
    int         m_ibMax;
    int         m_ibMin;
    QPixmap     m_imgBackground;
    QPixmap     m_imgObject;




signals:
    //=========================================================================================================
    /**
     * Emitted whenever the settings changed and are ready to be retreived.
     */
    void changeImgBackground(QPixmap ImgBackground);
    void changeImgObject(QPixmap ImgObject);
    void changeMax(int ibMax);
    void changeMin(int ibMin);



};
} // NAMESPACE

#endif // NEUROFEEDBACKBSETTINGSVIEW_H
