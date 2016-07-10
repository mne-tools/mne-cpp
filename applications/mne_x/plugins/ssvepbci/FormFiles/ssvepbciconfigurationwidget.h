//=============================================================================================================
/**
* @file     ssvepbciconfigurationwidget.h
* @author   Viktor Klüber <viktor.klueber@tu-ilmenau.de>;
*           Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     June 2016
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
* @brief    Contains the declaration of the ssvepBCIConfiguration class.
*
*/

#ifndef SSVEPBCICONFIGURATIONWIDGET_H
#define SSVEPBCICONFIGURATIONWIDGET_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================


#include "ssvepbciscreen.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDialog>
#include <QScreen>
#include <QWidget>
#include <QOpenGLWidget>
#include <QListWidgetItem>

//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE EEGoSportsPlugin
//=============================================================================================================

namespace Ui {
class ssvepBCIConfigurationWidget;
}

namespace ssvepBCIPlugin
{

//*************************************************************************************************************
//=============================================================================================================
// TypeDefs
//=============================================================================================================

typedef  QList<double>  MyQList;

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
class ssvepBCIConfigurationWidget : public QDialog
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
    explicit ssvepBCIConfigurationWidget(ssvepBCI* pssvepBCI, QWidget *parent = 0);

    //=========================================================================================================
    /**
    * Destructs a EEGoSportsSetupStimulusWidget which is a child of parent.
    *
    */
    ~ssvepBCIConfigurationWidget();

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
    * initialize selected channels-list on sensor level
    *
    *
    */
    void initSelectedChannelsSensor();

    //=========================================================================================================
    /**
    * updates the threshold values of the sliders and paints it to the screen
    */
    void updateThresholdsToScreen();

    //=========================================================================================================
    /**
    * returns the number of desired harmonics for which will be produced in the reference signal
    *
    * @return       number of desired harmonics
    */
    int getNumOfHarmonics();

    //=========================================================================================================
    /**
    * getting the string lists for senesor channel selection
    *
    */
    QStringList getSensorChannelSelection();

    //=========================================================================================================
    /**
    * getting the string lists for source channel selection
    *
    */
    QStringList getSourceChannelSelection();


public slots:
    void setSSVEPProbabilities(MyQList SSVEP);
    void setFrequencyList(MyQList frequencyList);
    void setClassResult(double classResult);

private slots:
    void on_m_RadioButton_MEC_toggled(bool checked);
    void thresholdChanged(double threshold);
    void numOfHarmonicsChanged(int harmonics);
    void channelSelectChanged(const QModelIndex &parent, int first, int last);

signals:
    void getThresholdValues(MyQList Thresholds);
    void changeSSVEPParameter();

private:
    Ui::ssvepBCIConfigurationWidget        *ui;                     /**< pointer to corresponding user interface */
    ssvepBCI                               *m_pSSVEPBCI;            /**< a pointer to corresponding ssvepBCI class */

    QStringList                             m_vAvailableChannelsSensor;                 /**< QStringList holding available features to select on sensor level (electrodes).*/
    double                                  m_dMinProbValue;                            /**< minimum border for SSVEP visualization with a status bar */
    double                                  m_dMaxProbValue;                            /**< maximum border for SSVEP visualization with a status bar */
    bool                                    m_bInitThresholdDisplay;                                    /**< flag for first run and so initializing ssvepBCIConfigurationWidget-Class */
    QList<double>                           m_lSSVEPThresholdValues;                    /**< contains the threshold values for SSVEP classifiaction */
    QList<double>                           m_lFrequencyList;                           /**< list of desired frequencies */
    QPalette                                m_palBlackFont;                             /**< setting black font for group box */
    QPalette                                m_palRedFont;                               /**< setting red font for highlightning label */




};

} //NAMESPACE

#endif // SSVEPBCICONFIGURATIONWIDGET_H
