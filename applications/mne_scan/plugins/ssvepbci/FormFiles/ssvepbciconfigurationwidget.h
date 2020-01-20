//=============================================================================================================
/**
 * @file     ssvepbciconfigurationwidget.h
 * @author   Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     June 2016
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
#include <QDateTime>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
class SsvepBciConfigurationWidget;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE SSVEPBCIPLUGIN
//=============================================================================================================

namespace SSVEPBCIPLUGIN
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

class SsvepBci;


//=============================================================================================================
/**
 * DECLARE CLASS SsvepBciConfigurationWidget
 *
 * @brief The SsvepBciConfigurationWidget class provides the EEGoSportsSetupStimulusWidget configuration window.
 */
class SsvepBciConfigurationWidget : public QDialog
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
    explicit SsvepBciConfigurationWidget(SsvepBci* pSsvepBci, QWidget *parent = 0);

    //=========================================================================================================
    /**
     * Destructs a EEGoSportsSetupStimulusWidget which is a child of parent.
     *
     */
    ~SsvepBciConfigurationWidget();

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
     */
    QStringList getSensorChannelSelection();

    //=========================================================================================================
    /**
     * getting the string lists for source channel selection
     */
    QStringList getSourceChannelSelection();

    //=========================================================================================================
    /**
     * resetting thresholdbars values to maximal statusbar borders
     */
    void resetThresholdValues();

public slots:
    //=========================================================================================================
    /**
     * sets the SSVEP probabilities and puts them to the GUI of the SsvepBciConfigurationWidget
     *
     * @param[in]    SSVEP   QList of all five SSVEP values
     */
    void setSSVEPProbabilities(MyQList SSVEP);

    //=========================================================================================================
    /**
     * sets the labels when the frequencies are changed
     *
     * @param[in]    frequencyList   frequencies which are looked for
     */
    void setFrequencyLabels(MyQList frequencyList);

    //=========================================================================================================
    /**
     * signalizes the classification result and marks it red to the screen
     *
     * @param[in]    classResult     classification result
     */
    void setClassResult(double classResult);

    //=========================================================================================================
    /**
     * slot for stopping the measurement of the accuracy
     */
    void stopMeasurement();

private slots:
    //=========================================================================================================
    /**
     * slot for adjusting MEC or CCA as extraction method which is connected to the MEC qRadioButton
     *
     * @param [in]   checked     unused paramater
     */
    void onRadioButtonMECtoggled(bool checked);

    //=========================================================================================================
    /**
     * slot for changing the thresholds
     *
     * @param [in]   threshodld     unused paramater
     */
    void thresholdChanged(double threshold);

    //=========================================================================================================
    /**
     * slot for changing the number of harmonics
     *
     * @param [in]   harmonics     number of harmonics
     */
    void numOfHarmonicsChanged(int harmonics);

    //=========================================================================================================
    /**
     * slot for changing the selected channels
     *
     * @param [in]   parent      unused parameter
     * @param [in]   first       unused parameter
     * @param [in]   last        unused parameter
     */
    void channelSelectChanged(const QModelIndex &parent, int first, int last);

    //=========================================================================================================
    /**
     * evaluates the classification result
     *
     * @param [in]   isCorrectCommand     flag when there was a classification
     */
    void evaluateCommand(bool isCorrectCommand);

    //=========================================================================================================
    /**
     * Slot for elapsed time. Triggered by a qTimer
     */
    void showCurrentTime();

    //=========================================================================================================
    /**
     * starts the accuracy measurement
     */
    void onStartMeasurementClicked();

    //=========================================================================================================
    /**
     * stops the accuracy measurement
     */
    void onStopMeasurementClicked();

    //=========================================================================================================
    /**
     * slot for changing the size of the classification list
     */
    void classificationListSizeChanged(int arg1);

signals:
    //=========================================================================================================
    /**
     * emit newly adjusted threshold values
     *
     * @param[out]    thresholds     classification result
     */
    void getThresholdValues(MyQList thresholds);

    //=========================================================================================================
    /**
     * signal for indicating a signal parameter change
     */
    void changeSSVEPParameter();

private:
    // main links
    Ui::SsvepBciConfigurationWidget*        ui;                     /**< Pointer to corresponding user interface. */
    SsvepBci*                               m_pSsvepBci;            /**< A pointer to corresponding SsvepBci class. */
    // configuration parameter
    QStringList                             m_vAvailableChannelsSensor;                 /**< QStringList holding available features to select on sensor level (electrodes).*/
    double                                  m_dMinProbValue;                            /**< Minimum border for SSVEP visualization with a status bar. */
    double                                  m_dMaxProbValue;                            /**< Maximum border for SSVEP visualization with a status bar. */
    bool                                    m_bInitThresholdDisplay;                    /**< Flag for first run and so initializing SsvepBciConfigurationWidget-Class. */
    QList<double>                           m_lSSVEPThresholdValues;                    /**< Contains the threshold values for SSVEP classifiaction. */
    QList<double>                           m_lFrequencyList;                           /**< List of desired frequencies. */
    QPalette                                m_palBlackFont;                             /**< Setting black font for group box. */
    QPalette                                m_palRedFont;                               /**< Setting red font for highlightning label. */
    // accuracy feature
    bool                                    m_bScreenKeyboardConnected;                 /**< Flag for connected screen keaboard. */
    int                                     m_iCorrectCommands;                         /**< Counter for correct commands. */
    int                                     m_iWrongCommands;                           /**< Counter for wrong commands. */
    QTimer*                                 m_qTimer;                                   /**< Timer for displaying accuracy every second. */
    int                                     m_iElapsedSeconds;                          /**< Time, displayed on screen. */
};

} //NAMESPACE

#endif // SSVEPBCICONFIGURATIONWIDGET_H
