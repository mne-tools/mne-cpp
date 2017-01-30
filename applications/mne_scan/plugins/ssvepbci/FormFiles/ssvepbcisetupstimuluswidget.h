//=============================================================================================================
/**
* @file     ssvepbcisetupstimuluswidget.h
* @author   Viktor Klüber <viktor.klueber@tu-ilmenau.de>;
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     April, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Viktor Klüber Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the ssvepbcisetupstimulus class.
*
*/

#ifndef SSVEPBCISETUPSTIMULUSWIDGET_H
#define SSVEPBCISETUPSTIMULUSWIDGET_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../ssvepbci.h"
#include "screenkeyboard.h"
#include "ssvepbciscreen.h"
#include "ssvepbciflickeringitem.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDialog>
#include <QScreen>
#include <QWidget>
#include <QOpenGLWidget>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
class SsvepBciSetupStimulusWidget;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE SSVEPBCIPLUGIN
//=============================================================================================================

namespace SSVEPBCIPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class SsvepBci;
class SsvepBciScreen;
class SsvepBciFlickeringItem;
class ScreenKeyboard;


//=============================================================================================================
/**
* DECLARE CLASS SsvepBciSetupStimulusWidget
*
* @brief The SsvepBciSetupStimulusWidget class provides the SsvepBciSetupStimulusWidget configuration window.
*/
class SsvepBciSetupStimulusWidget : public QDialog
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
    * Constructs a c which is a child of parent.
    *
    * @param [in] parent        a pointer to parent widget; If parent is 0, the new EEGoSportsSetupStimulusWidget becomes a window. If parent is another widget, EEGoSportsSetupStimulusWidget becomes a child window inside parent. EEGoSportsSetupStimulusWidget is deleted when its parent is deleted.
    * @param [in] pSsvepBci     a pointer to the corresponding ECGSimulator.
    */
    explicit SsvepBciSetupStimulusWidget(SsvepBci *pSsvepBci, QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destructs a EEGoSportsSetupStimulusWidget which is a child of parent.
    *
    */
    ~SsvepBciSetupStimulusWidget();

    //=========================================================================================================
    /**
    * clears the QGraphicsScene from all Items.
    *
    */
    void clear();

    //=========================================================================================================
    /**
    * Close event, when setup-stimulus window is closed.
    *
    * @param [in] QClosEvent for clsoing the window
    *
    */
    void closeEvent(QCloseEvent *event);

    //=========================================================================================================
    /**
    * Gets the list of all displayed freuqnecies
    *
    * @return  list of all displayed frequencies
    *
    */
    QList<double> getFrequencies();

    //=========================================================================================================
    /**
    * gets pointer to private ScreenKeyboard-object
    *
    * @return  QSharedPointer to ScreenKeaboard-object
    *
    */
    QSharedPointer<ScreenKeyboard> getScreenKeyboardSPtr();

signals:
    //=========================================================================================================
    /**
    * signal for indicating a signal change
    */
    void frequencyChanged();

    //=========================================================================================================
    /**
    * signal for indicating a text change
    */
    void settledPhrase(QString phrase);

private slots:
    //=========================================================================================================
    /**
    * Shows the widget on Fullscreen.
    */
    void showTestScreen();

    //=========================================================================================================
    /**
    * Clears the blinking items from screen.
    */
    void clearItems();

    //=========================================================================================================
    /**
    * Sets the state of the window to minimized.
    */
    void minimizeScreen();

    //=========================================================================================================
    /**
    * starts test 3
    */
    void test3();

    //=========================================================================================================
    /**
    * starts test 1
    *
    */
    void test1();

    //=========================================================================================================
    /**
    * starts test 2
    *
    */
    void test2();

    //=========================================================================================================
    /**
    * choose Item for frequency reaading and writing
    *
    * @param [in] index value of the Item List
    *
    */
    void panelSelect(int index);

    //=========================================================================================================
    /**
    * choose Item for frequency reaading and writing
    *
    * @param [in] index Get the frequencie's index of of the selected item.
    *
    */
    void frequencySelect(int index);

    //=========================================================================================================
    /**
    * Changes the render order (frequency) of the selected item.
    *
    * @param [in]   item        Selected Item.
    * @param [in]   freqKey     Frequency key of the Item.
    *
    */
    void setFreq(SsvepBciFlickeringItem &item, int freqKey);

    //=========================================================================================================
    /**
    * Starts the Screen Keyboard device.
    */
    void screenKeyboard();

    //=========================================================================================================
    /**
    * Changes the render order (frequency) of the selected item.
    *
    * @param [in]   arg1        Edit the spell text.
    *
    */
    void on_m_lineEdit_BCISpeller_textChanged(const QString &arg1);

private:
    //=========================================================================================================
    /**
    * adapts the Item list of the combox box automatically
    *
    * @param [in] QList of all FlashObject on screen
    *
    */
    void changeComboBox();

    Ui::SsvepBciSetupStimulusWidget*        ui;                     /**< Pointer to the graphical user interface. */
    QSharedPointer<SsvepBci>                m_pSsvepBci;            /**< a pointer to corresponding EEGoSports */
    QSharedPointer<SsvepBciScreen>          m_pSsvepBciScreen;      /**< pointer to the SsvepBciscreen class of the subject (friend class) */
    QSharedPointer<ScreenKeyboard>          m_pScreenKeyboard;      /**< pointer to the Screenkeyboard class */
    QSharedPointer<QScreen>                 m_pScreen;              /**< pointer to the QScreen class; */
    bool                                    m_bIsRunning;           /**< Flag for running test */
    bool                                    m_bReadFreq;            /**< Flag for reading the adjusted frequency */
    QMap<int, double>                       m_idFreqMap;            /**< containing frequencies and their corresponding key */
};

} //NAMESPACE

#endif // SSVEPBCISETUPSTIMULUSWIDGET_H
