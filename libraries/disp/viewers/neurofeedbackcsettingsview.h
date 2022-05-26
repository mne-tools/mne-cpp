//=============================================================================================================
/**
 * @file     neurofeedbackcsettingsview.h
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
 * @brief    Declaration of the NeurofeedbackCSettingsView Class.
 *
 */

#ifndef NEUROFEEDBACKCSETTINGSVIEW_H
#define NEUROFEEDBACKCSETTINGSVIEW_H

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
class NeurofeedbackCSettingsViewWidget;
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
 * DECLARE CLASS NeurofeedbackCSettingsView
 */
class DISPSHARED_EXPORT NeurofeedbackCSettingsView : public QWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<NeurofeedbackCSettingsView> SPtr;              /**< Shared pointer type for NeurofeedbackCSettingsView. */
    typedef QSharedPointer<const NeurofeedbackCSettingsView> ConstSPtr;   /**< Const shared pointer type for NeurofeedbackCSettingsView. */

    //=========================================================================================================
    /**
     * Constructs a NeurofeedbackCSettingsView which is a child of parent.
     *
     * @param [in] parent    parent of widget
     */
    NeurofeedbackCSettingsView(const QString& sSettingsPath = "",
                          const QString& sClass0 = QString::fromStdString("Class0"),
                          const QString& sClass1 = QString::fromStdString("Class1"),
                          const QString& sClass2 = QString::fromStdString("Class2"),
                          const QString& sClass3 = QString::fromStdString("Class3"),
                          const QString& sClass4 = QString::fromStdString("Class4"),
                          int iClass0 = 0,
                          int iClass1 = 1,
                          int iClass2 = 2,
                          int iClass3 = 3,
                          int iClass4 = 4,
                          int iNumbofClass = 2,
                          bool bIsRunning = false,
                          QWidget *parent = 0,
                          Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================

    ~NeurofeedbackCSettingsView();

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
     * Slot called when number of classifications spin box changes
     *
     * @param[in] value        number of classifications.
     */
    void onUpdateSpinBoxNumbofClass(int value);

    //=========================================================================================================
    /**
     * Slot called when Class0 line edit changes
     */
    void onUpdateLineEditClass0();

    //=========================================================================================================
    /**
     * Slot called when Class1 line edit changes
     */
    void onUpdateLineEditClass1();

    //=========================================================================================================
    /**
     * Slot called when Class2 line edit changes
     */
    void onUpdateLineEditClass2();

    //=========================================================================================================
    /**
     * Slot called when Class3 line edit changes
     */
    void onUpdateLineEditClass3();

    //=========================================================================================================
    /**
     * Slot called when Class4 line edit changes
     */
    void onUpdateLineEditClass4();

    //=========================================================================================================
    /**
     * Slot called when Class0 spin box changes
     *
     * @param[in] value        Integer Class0.
     */
    void onUpdateSpinboxClass0(int value);

    //=========================================================================================================
    /**
     * Slot called when Class1 spin box changes
     *
     * @param[in] value        Integer Class0.
     */
    void onUpdateSpinboxClass1(int value);

    //=========================================================================================================
    /**
     * Slot called when Class2 spin box changes
     *
     * @param[in] value        Integer Class0.
     */
    void onUpdateSpinboxClass2(int value);

    //=========================================================================================================
    /**
     * Slot called when Class3 spin box changes
     *
     * @param[in] value        Integer Class0.
     */
    void onUpdateSpinboxClass3(int value);

    //=========================================================================================================
    /**
     * Slot called when Class4 spin box changes
     *
     * @param[in] value        Integer Class0.
     */
    void onUpdateSpinboxClass4(int value);

    //=========================================================================================================
    /**
     * Slot called when Class0 Button Group changes
     *
     * @param[in] value        Integer for Color Class0.
     */
    void onUpdateButtonGroupC0(int value);

    //=========================================================================================================
    /**
     * Slot called when Class1 Button Group changes
     *
     * @param[in] value        Integer for Color Class0.
     */
    void onUpdateButtonGroupC1(int value);

    //=========================================================================================================
    /**
     * Slot called when Class2 Button Group changes
     *
     * @param[in] value        Integer for Color Class0.
     */
    void onUpdateButtonGroupC2(int value);

    //=========================================================================================================
    /**
     * Slot called when Class3 Button Group changes
     *
     * @param[in] value        Integer for Color Class0.
     */
    void onUpdateButtonGroupC3(int value);

    //=========================================================================================================
    /**
     * Slot called when Class4 Button Group changes
     *
     * @param[in] value        Integer for Color Class0.
     */
    void onUpdateButtonGroupC4(int value);

    //=========================================================================================================
    /**
     * Slot is called when Load Button for Classification 0 is clicked
     */
    void clickedLoadButton0();

    //=========================================================================================================
    /**
     * Slot is called when Load Button for Classification 1 is clicked
     */
    void clickedLoadButton1();

    //=========================================================================================================
    /**
     * Slot is called when Load Button for Classification 2 is clicked
     */
    void clickedLoadButton2();

    //=========================================================================================================
    /**
     * Slot is called when Load Button for Classification 3 is clicked
     */
    void clickedLoadButton3();

    //=========================================================================================================
    /**
     * Slot is called when Load Button for Classification 4 is clicked
     */
    void clickedLoadButton4();

    //=========================================================================================================
    /**
     * Slot is called when Delete Button for Classification 0 is clicked
     */
    void clickedDeleteButton0();

    //=========================================================================================================
    /**
     * Slot is called when Delete Button for Classification 1 is clicked
     */
    void clickedDeleteButton1();

    //=========================================================================================================
    /**
     * Slot is called when Delete Button for Classification 2 is clicked
     */
    void clickedDeleteButton2();

    //=========================================================================================================
    /**
     * Slot is called when Delete Button for Classification 3 is clicked
     */
    void clickedDeleteButton3();

    //=========================================================================================================
    /**
     * Slot is called when Delete Button for Classification 4 is clicked
     */
    void clickedDeleteButton4();

    //=========================================================================================================
    /**
     * Slot is called when Reset Settings Button is clicked
     */
    void clickedResetSettings();

    //=========================================================================================================
    /**
     * Update all Settings on the NeurofeedbackCSettingsView Display.
     */
    void updateDisplay();

    //=========================================================================================================
    /**
     * Update the Image for Classification 0.
     */
    void updateImgClass0();

    //=========================================================================================================
    /**
     * Update the Image for Classification 1.
     */
    void updateImgClass1();

    //=========================================================================================================
    /**
     * Update the Image for Classification 2.
     */
    void updateImgClass2();

    //=========================================================================================================
    /**
     * Update the Image for Classification 3.
     */
    void updateImgClass3();

    //=========================================================================================================
    /**
     * Update the Image for Classification 4.
     */
    void updateImgClass4();


    Ui::NeurofeedbackCSettingsViewWidget *m_pui;

    QString                 m_sSettingsPath;
    QString                 m_sClass0;
    QString                 m_sClass1;
    QString                 m_sClass2;
    QString                 m_sClass3;
    QString                 m_sClass4;
    QString                 m_sDirClass0;
    QString                 m_sDirClass1;
    QString                 m_sDirClass2;
    QString                 m_sDirClass3;
    QString                 m_sDirClass4;
    QString                 m_sColClass0;
    QString                 m_sColClass1;
    QString                 m_sColClass2;
    QString                 m_sColClass3;
    QString                 m_sColClass4;

    int                     m_iClass0;
    int                     m_iClass1;
    int                     m_iClass2;
    int                     m_iClass3;
    int                     m_iClass4;
    int                     m_iNumbofClass;

    bool                    m_bIsRunning;

    QPixmap                 m_imgClass0;
    QPixmap                 m_imgClass1;
    QPixmap                 m_imgClass2;
    QPixmap                 m_imgClass3;
    QPixmap                 m_imgClass4;

    QVector<int>            m_vintClass;
    QVector<QLineEdit*>     m_vLineEditsClass;
    QVector<QLabel*>        m_vLabelClass;
    QVector<QSpinBox*>      m_vSpinboxClass;
    QVector<QLineEdit*>     m_vLineEditsDirClass;
    QVector<QPushButton*>   m_vPushButtonsClass;
    QVector<QLabel*>        m_vLabelStatusClass;
    QVector<QLabel*>        m_vLabelImgClass;
    QVector<QRadioButton*>  m_vRadioButtonClassg;
    QVector<QRadioButton*>  m_vRadioButtonClassb;
    QVector<QPushButton*>   m_vDeleteButtonsClass;


signals:
    //=========================================================================================================
    /**
     * Emitted whenever the settings changed and are ready to be retreived.
     */

    void changeNumbofClass(int value);
    void changeClass0(const QString& sClass0);
    void changeClass1(const QString& sClass1);
    void changeClass2(const QString& sClass2);
    void changeClass3(const QString& sClass3);
    void changeClass4(const QString& sClass4);
    void changeButtonGroupC0(const QString& sBGC0);
    void changeButtonGroupC1(const QString& sBGC1);
    void changeButtonGroupC2(const QString& sBGC2);
    void changeButtonGroupC3(const QString& sBGC3);
    void changeButtonGroupC4(const QString& sBGC4);
    void changeiClass0(int value);
    void changeiClass1(int value);
    void changeiClass2(int value);
    void changeiClass3(int value);
    void changeiClass4(int value);
    void changeImgClass0(QPixmap imgClass0);
    void changeImgClass1(QPixmap imgClass1);
    void changeImgClass2(QPixmap imgClass2);
    void changeImgClass3(QPixmap imgClass3);
    void changeImgClass4(QPixmap imgClass4);

};
} // NAMESPACE

#endif // NEUROFEEDBACKCSETTINGSVIEW_H
