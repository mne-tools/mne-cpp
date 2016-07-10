//=============================================================================================================
/**
* @file     ssvepbcisetupstimulus.h
* @author   Viktor Klüber <viktor.klueber@tu-ilmenau.de>;
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     April 2016
*
* @section  LICENSE
*
* Copyright (C) 2014, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
#include "ssvepbciscreen.h"


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
// DEFINE NAMESPACE EEGoSportsPlugin
//=============================================================================================================

namespace Ui {
class ssvepBCISetupStimulusWidget;
}

namespace ssvepBCIPlugin
{


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class ssvepBCI;


//=============================================================================================================
/**
* DECLARE CLASS EEGoSportsSetupStimulusWidget
*
* @brief The EEGoSportsSetupStimulusWidget class provides the EEGoSportsSetupStimulusWidget configuration window.
*/
class ssvepBCISetupStimulusWidget : public QDialog
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
    * Constructs a c which is a child of parent.
    *
    * @param [in] parent pointer to parent widget; If parent is 0, the new EEGoSportsSetupStimulusWidget becomes a window. If parent is another widget, EEGoSportsSetupStimulusWidget becomes a child window inside parent. EEGoSportsSetupStimulusWidget is deleted when its parent is deleted.
    * @param [in] pEEGoSports a pointer to the corresponding ECGSimulator.
    */
    explicit ssvepBCISetupStimulusWidget(ssvepBCI* pssvepBCI, QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destructs a EEGoSportsSetupStimulusWidget which is a child of parent.
    *
    */
    ~ssvepBCISetupStimulusWidget();

    //=========================================================================================================
    /**
    * clears the QGraphicsScene from all Items.
    *
    */
    void clear();

    //=========================================================================================================
    /**
    * close event, when setup-stimulus window is closed.
    *
    * @param [in] QClosEvent for clsoing the window
    *
    */
    void closeEvent(QCloseEvent *event);

    //=========================================================================================================
    /**
    * gets the list of all displayed freuqnecies
    *
    * @return  list of all displayed frequencies
    *
    */
    QList<double> getFrequencies();

signals:
    void frequencyChanged();

private slots:


    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();


    //=========================================================================================================
    /**
    * start a test
    *
    */
    void on_pushButton_4_clicked();

    //=========================================================================================================
    /**
    * start a test
    *
    */
    void on_pushButton_5_clicked();

    //=========================================================================================================
    /**
    * start a test
    *
    */
    void on_pushButton_6_clicked();

    //=========================================================================================================
    /**
    * choose Item for frequency reaading and writing
    *
    * @param [in] index value of the Item List
    *
    */
    void on_comboBox_currentIndexChanged(int index);

    void on_comboBox_2_currentIndexChanged(int index);

    void setFreq(ssvepBCIFlickeringItem &item, int freqKey);

    void on_pushButton_7_clicked();

private:
    Ui::ssvepBCISetupStimulusWidget        *ui;
    ssvepBCI                               *m_pssvepBCI;            /**< a pointer to corresponding EEGoSports */
    ssvepBCIScreen                         *m_pssvepBCIScreen;      /**< pointer to the screen class of the subject (friend class) */
    bool                                    m_bIsRunning;           /**< Flag for running test */
    bool                                    m_bReadFreq;            /**< Flag for reading the adjusted frequency */
    QMap<int, double>                       m_idFreqMap;            /**< containing frequencies and their corresponding key */

    //=========================================================================================================
    /**
    * adapts the Item list of the combox box automatically
    *
    * @param [in] QList of all FlashObject on screen
    *
    */
    void changeComboBox();
};

} //NAMESPACE

#endif // SSVEPBCISETUPSTIMULUSWIDGET_H
