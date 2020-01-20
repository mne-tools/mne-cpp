//=============================================================================================================
/**
 * @file     ssvepbci.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @version  dev
 * @date     May, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Christoph Dinh, Lorenz Esch, Viktor Klueber. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Contains the declaration of the ssvepBCI class.
 *
 */

#ifndef SSVEPBCI_H
#define SSVEPBCI_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ssvepbci_global.h"

#include <scShared/Interfaces/IAlgorithm.h>
#include <utils/generics/circularmatrixbuffer.h>
#include <scMeas/realtimesamplearray.h>
#include <scMeas/realtimemultisamplearray.h>
#include <scMeas/realtimesourceestimate.h>
#include <utils/filterTools/filterdata.h>

#include <fstream>
#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <Eigen/Dense>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QtConcurrent/QtConcurrent>

#include "FormFiles/ssvepbciwidget.h"
#include "FormFiles/ssvepbciconfigurationwidget.h"
#include "FormFiles/ssvepbcisetupstimuluswidget.h"


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

class SsvepBciSetupStimulusWidget;
class SsvepBciScreen;
class SsvepBciConfigurationWidget;


//=============================================================================================================
/**
 * Main SsvepBci class which contains the detection algorithm of SSVEPs
 *
 * @brief The BCI class provides an EEG SSVEP-BCI.
 */
class SSVEPBCISHARED_EXPORT SsvepBci : public SCSHAREDLIB::IAlgorithm
{

    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "ssvepbci.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro

    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::IAlgorithm)

    friend class SsvepBciConfigurationWidget;

public:
    //=========================================================================================================
    /**
    * Constructs a SSVEP BCI.
    */
    SsvepBci();

    //=========================================================================================================
    /**
    * Destroys the BCI.
    */
    virtual ~SsvepBci();

    //=========================================================================================================
    /**
    * Clone the plugin
    */
    virtual QSharedPointer<SCSHAREDLIB::IPlugin> clone() const;

    //=========================================================================================================
    /**
    * Initialise input and output connectors.
    */
    virtual void init();

    //=========================================================================================================
    /**
    * Shows Setup of Stimulus Feature.
    */
    void showSetupStimulus();

    //=========================================================================================================
    /**
    * Shows configuration panel of BCI.
    */
    void showBCIConfiguration();

    //=========================================================================================================
    /**
    * Is called when plugin is detached of the stage. Can be used to safe settings.
    */
    virtual void unload();

    //=========================================================================================================
    /**
    * Starts the BCI by starting the BCI's thread.
    */
    virtual bool start();

    //=========================================================================================================
    /**
    * Stops the BCI by stopping the BCI's thread.
    */
    virtual bool stop();

    //=========================================================================================================
    /**
    * returns the plug-in type
    */
    virtual SCSHAREDLIB::IPlugin::PluginType getType() const;

    //=========================================================================================================
    /**
     * return the name of the plug-in
     */
    virtual QString getName() const;

    //=========================================================================================================
    /**
     * intializing the setup of the widget.
     */
    virtual QWidget* setupWidget();

    //=========================================================================================================
    /**
    * get a current list of frquencies which are examined.
    *
    * @return       list of all desired frequencies
    */
    QList<double>  getCurrentListOfFrequencies();

    //=========================================================================================================
    /**
    * retruns the ssvep BCI resource path
    *
    * @return       string of the resource path
    */
    QString getSsvepBciResourcePath();

protected:
    /**
    * This update function gets called whenever the input buffer stream from the TMSI plugin is full and need to be emptied by this BCI plugin.
    *
    * @param [in] pMeasurement measurement object.
    */
    void updateSensor(SCMEASLIB::Measurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
    * This update function gets called whenever the input buffer stream from the Sourcelab plugin is full and need to be emptied by this BCI plugin.
    *
    * @param [in] pMeasurement measurement object.
    */
    void updateSource(SCMEASLIB::Measurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
    * Clears all values from the list of classficiation results. Normally after a SSVEP has been detected
    *
    */
    void clearClassifications();


    //=========================================================================================================
    /**
    * Applying the Minimum Energy Combination approach in order to get the signal energy in Y detected by the
    * reference signal X.
    *
    * @param [in]   Y           measured signal.
    * @param [in]   X           reference signal.
    *
    * @return       signal energy of the reference signal in the measured signal.
    *
    */
    double MEC(SCMEASLIB::MatrixXd &Y, SCMEASLIB::MatrixXd &X);

    //=========================================================================================================
    /**
    * Applying Canoncial Correlation Analysis to get the correlation between the sets of signals of reference
    * Signal X and the EEG Signal Y.
    *
    * @param [in]   Y           measured signal.
    * @param [in]   X           reference signal.
    *
    * @return       maximal correlation between the signals.
    *
    */
    double CCA(SCMEASLIB::MatrixXd &Y, SCMEASLIB::MatrixXd &X);

    //=========================================================================================================
    /**
    * The starting point for the thread. After calling start(), the newly created thread calls this function.
    * Returning from this method will end the execution of the thread.
    * Pure virtual method inherited by QThread.
    */
    virtual void run();

    //=========================================================================================================
    /**
    * Executing the whole SSVEP-BCI on sensor level.
    */
    void ssvepBciOnSensor();

    //=========================================================================================================
    /**
    * Executing the whole SSVEP-BCI on sensor level.
    */
    void ssvepBciOnSource();

public slots:
    //=========================================================================================================
    /**
    * setting the flag for removing the power line
    *
    * @param [in]   removePowerline     Flag for power line
    */
    void removePowerLine(bool removePowerLine);

    //=========================================================================================================
    /**
    * adjusting the powerline frequency
    *
    * @param [in]   powerLine     frequency of the power line
    */
    void setPowerLine(int powerLine);

    //=========================================================================================================
    /**
    * slot adjusting the kind of feature extraction
    *
    * @param [in]   useMEC     flag for using MEC
    */
    void setFeatureExtractionMethod(bool useMEC);

    //=========================================================================================================
    /**
    * slot for adjusting the kind of feature extraction
    *
    * @param [in]   useMEC     flag for using MEC
    */
    void setThresholdValues(MyQList thresholds);

    //=========================================================================================================
    /**
    * slot for changing the SSVEP parameter
    */
    void setChangeSSVEPParameterFlag();

    //=========================================================================================================
    /**
    * slot for adjusting the number of classification hits
    *
    * @param [in]   numClassHits     number of classification hits
    */
    void setNumClassHits(int numClassHits);

    //=========================================================================================================
    /**
    * slot for setting the number of classfication breaks to the next classification process
    *
    * @param [in]   numClassBreaks     number of classification breaks
    */
    void setNumClassBreaks(int numClassBreaks);

    //=========================================================================================================
    /**
    * slot for setting the size of the classification list
    *
    * @param [in]   classListSize     size of the classification list
    */
    void setSizeClassList(int classListSize);

signals:
    //=========================================================================================================
    /**
    * emits the values of SSVEP probabilities
    *
    * @param [in]   ssvepProb     list of SSVEP frequencies
    */
    void SSVEPprob(MyQList ssvepProb);

    //=========================================================================================================
    /**
    * emits the classification result
    *
    * @param [in]   classResult     classification result
    */
    void classificationResult(double classResult);

    //=========================================================================================================
    /**
    * emits the the values of detectable frequencies
    *
    * @param [in]   frequencyList     list of SSVEP frequencies
    */
    void getFrequencyLabels(MyQList frequencyList);

private:    
    //=========================================================================================================
    /**
    * Reading actual segment from the sliding time window and write it to a data Matrix.
    *
    * @param [out] data      data space where current data from the sliding time window will be written to.
    */
    void readFromSlidingTimeWindow(SCMEASLIB::MatrixXd &data);

    //=========================================================================================================
    /**
    * Updates the parameter of the classifiaction process and resets the time window. This function is called
    * at the end of the algorithm.
    */
    void changeSSVEPParameter();

    //=========================================================================================================
    /**
    * Updates the list of desired frequencies and of all frequencies which will be examined.
    *
    * @param [in]   frequencyList   List of new frequencies.
    *
    */
    void setFrequencyList(QList<double> frequencyList);

    QAction*                                            m_pActionBCIConfiguration;          /**< Start configuration feature. */
    QAction*                                            m_pActionSetupStimulus;             /**< Starts stimulus feature. */
    QSharedPointer<SsvepBciConfigurationWidget>         m_pSsvepBciConfigurationWidget;     /**< Holds pointer to Widget for BCI configuration. */
    QSharedPointer<SsvepBciSetupStimulusWidget>         m_pSsvepBciSetupStimulusWidget;     /**< Widget for stimulus setup */

    SCSHAREDLIB::PluginInputData<SCMEASLIB::RealTimeMultiSampleArray>::SPtr  m_pRTMSAInput;          /**< The RealTimeMultiSampleArray input.*/
    SCSHAREDLIB::PluginInputData<SCMEASLIB::RealTimeSourceEstimate>::SPtr       m_pRTSEInput;           /**< The RealTimeSourceEstimate input.*/

    IOBUFFER::CircularMatrixBuffer<double>::SPtr                  m_pBCIBuffer_Sensor;    /**< Holds incoming sensor level data.*/
    IOBUFFER::CircularMatrixBuffer<double>::SPtr                  m_pBCIBuffer_Source;    /**< Holds incoming source level data.*/

    // processing parameter
    bool                    m_bUseSensorData;                   /**< GUI input: Use sensor data stream. */
    std::ofstream           m_outStreamDebug;                   /**< Outputstream to generate debug file.*/
    bool                    m_bIsRunning;                       /**< Whether BCI is running.*/
    QString                 m_qStringResourcePath;              /**< The path to the BCI resource directory.*/
    bool                    m_bProcessData;                     /**< Whether BCI is to get data out of the continous input data stream, i.e. the EEG data from sensor level.*/
    QMutex                  m_qMutex;                           /**< QMutex to guarantee thread safety.*/

    // adaptable sliding time window with downsampling function
    SCMEASLIB::MatrixXd     m_matSlidingTimeWindow;             /**< Sensor Level: adaptational sliding time window. */
    int                     m_iCounter;                         /**< iterative index for counting the amount of misclassifications */
    double                  m_dSampleFrequency;                 /**< sample frequency of the device [Hz] */
    int                     m_iReadSampleSize;                  /**< numbers of sample for one time segment (about 0.1 seconds) */
    int                     m_iWriteSampleSize;                 /**< numbers of sample for writing to the time window  */
    int                     m_iTimeWindowSegmentSize;           /**< needed size of the buffer for reading with an adaptable sliding window */
    int                     m_iTimeWindowLength;                /**< required length of the time window */
    int                     m_iWriteIndex;                      /**< Index for writing a new increment from the buffer to the time window */
    int                     m_iReadIndex;                       /**< index for reading from the time window */
    int                     m_iDownSampleIncrement;             /**< Increment for downsampling from current sample rate to 128 Hz */
    int                     m_iDownSampleIndex;                 /**< index for reading from the raw buffer in order to downsample to 128 Hz */
    int                     m_iFormerDownSampleIndex;           /**< former downsampling Index: serves as comparison variable for handling storage overflow */
    int                     m_iReadToWriteBuffer;               /**< number of samples from the current readindex to current write index */
    int                     m_iNumberOfClassBreaks;             /**< number of classifiactions whicht will be skipped if a classifiaction was made */
    int                     m_iWindowSize;                      /**< size of current time window */
    // SSVEP parameter
    QList<int>              m_lElectrodeNumbers;                /**< Sensor level: numbers of chosen electrode channels. */
    QList<double>           m_lDesFrequencies;                  /**< Contains desired frequencies. */
    QList<double>           m_lAllFrequencies;                  /**< Contains desired frequencies and reference frequencies. */
    int                     m_iNumberOfHarmonics;               /**< Number of harmonics which will be searched for. */
    double                  m_dAlpha;                           /**< Parameter for softmax function. */
    QList<double>           m_lThresholdValues;                 /**< Threshold value for normalized energy probabilities. */
    bool                    m_bRemovePowerLine;                 /**< Flag for removing 50 Hz power line signal. */
    bool                    m_bUseMEC;                          /**< Flag for feature extractiong. If true: use MEC; If false: use CCA. */
    QList<int>              m_lIndexOfClassResultSensor;        /**< Sensor level: Classification results on sensor level. */
    int                     m_iPowerLine;                       /**< Frequency of the power line [Hz]. */
    bool                    m_bChangeSSVEPParameterFlag;        /**< Flag for chaning SSVEP parameter. */
    int                     m_iNumberOfClassHits;               /**< Number of required classifiaction hits, before a classifiaction is confirmed. */
    int                     m_iClassListSize;                   /**< maximum size of m_lIndexOfClassResultSensor. */

    // Sensor level
    SCMEASLIB::FiffInfo::SPtr   m_pFiffInfo_Sensor;                 /**< Sensor level: Fiff information for sensor data. */
    QStringList                 m_slChosenChannelsSensor;           /**< Sensor level: Features used to calculate data points in feature space on sensor level. */
    QMap<QString, int>          m_mapElectrodePinningScheme;        /**< Sensor level: Loaded pinning scheme of the Duke 64 Dry EEG cap. */

    // Source level
    QVector< SCMEASLIB::VectorXd >      m_vLoadedSourceBoundary;            /**< Source level: Loaded decision boundary on source level. */
    QStringList                         m_slChosenChannelsSource;           /**< Source level: Features used to calculate data points in feature space on source level. */
    QMap<QString, int>                  m_mapDestrieuxAtlasRegions;         /**< Source level: Loaded Destrieux atlas regions. */
    bool                                m_bInitializeSource;                /**< Source level: initalizie parameter for processing on source level. */

    // output
    SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeSampleArray>::SPtr      m_pBCIOutputOne;        /**< The first RealTimeSampleArray of the BCI output.*/
    SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeSampleArray>::SPtr      m_pBCIOutputTwo;        /**< The second RealTimeSampleArray of the BCI output.*/
    SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeSampleArray>::SPtr      m_pBCIOutputThree;      /**< The third RealTimeSampleArray of the BCI output.*/
    SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeSampleArray>::SPtr      m_pBCIOutputFour;       /**< The fourth RealTimeSampleArray of the BCI output.*/
    SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeSampleArray>::SPtr      m_pBCIOutputFive;       /**< The fifth RealTimeSampleArray of the BCI output.*/

};

}//NAMESPACE

#endif //SSVEPBCI_H
