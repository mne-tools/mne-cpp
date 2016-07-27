//=============================================================================================================
/**
* @file     ssvepbci.h
* @author   Viktor Klüber <viktor.klueber@tu-ilmenau.de>;
*           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     May, 2016
*
* @section  LICENSE
*
* Copyright (C) 2013, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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

#include <generics/circularmatrixbuffer.h>

#include <scMeas/newrealtimesamplearray.h>
#include <scMeas/newrealtimemultisamplearray.h>
#include <scMeas/realtimesourceestimate.h>

#include <utils/FilterTools/filterdata.h>

#include <fstream>

#include <iostream>
#include <Eigen/Dense>

//*************************************************************************************************************
//=============================================================================================================
// QT STL INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QtConcurrent/QtConcurrent>

#include "FormFiles/ssvepbciwidget.h"
#include "FormFiles/ssvepbciconfigurationwidget.h"
#include "FormFiles/ssvepbcisetupstimuluswidget.h"

//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE TMSIPlugin
//=============================================================================================================

namespace ssvepBCIPlugin
{

//*************************************************************************************************************
//=============================================================================================================
// TypeDefs
//=============================================================================================================

typedef  QList<double>  MyQList;

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace IOBuffer;
using namespace UTILSLIB;
using namespace FSLIB;
using namespace std;


//=============================================================================================================
/**
* BCI...
*
* @brief The BCI class provides an EEG BCI.
*/
class SSVEPBCISHARED_EXPORT ssvepBCI : public IAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "mne_scan/1.0" FILE "ssvepbci.json") //NEw Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::IAlgorithm)

    friend class BCISetupWidget;
    friend class BCIFeatureWindow;
    friend class ssvepBCISetupStimulusWidget;
    friend class ssvepBCIConfigurationWidget;
    friend class ssvepBCIWidget;

public:
    //=========================================================================================================
    /**
    * Constructs a BCI.
    */
    ssvepBCI();

    //=========================================================================================================
    /**
    * Destroys the BCI.
    */
    virtual ~ssvepBCI();

    //=========================================================================================================
    /**
    * Clone the plugin
    */
    virtual QSharedPointer<IPlugin> clone() const;

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

    virtual IPlugin::PluginType getType() const;
    virtual QString getName() const;
    virtual QWidget* setupWidget();

    //=========================================================================================================
    /**
    * get a current list of frquencies which are looked for
    *
    * @return       list of all desired frequencies
    */
    QList<double>  getCurrentListOfFrequencies();

    //=========================================================================================================
    /**
    * retruns the ssvep BCI resource path
    *
    */
    QString getSSVEPBCIResourcePath();


protected:
    /**
    * This update function gets called whenever the input buffer stream from the TMSI plugin is full and need to be emptied by this BCI plugin.
    *
    * @param [in] pMeasurement measurement object.
    */
    void updateSensor(SCMEASLIB::NewMeasurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
    * This update function gets called whenever the input buffer stream from the Sourcelab plugin is full and need to be emptied by this BCI plugin.
    *
    * @param [in] pMeasurement measurement object.
    */
    void updateSource(SCMEASLIB::NewMeasurement::SPtr pMeasurement);


    //=========================================================================================================
    /**
    * Clears all classification results
    *
    */
    void clearClassifications();


    //=========================================================================================================
    /**
    * Look for trigger in stim channel
    *
    */
    bool lookForTrigger(const MatrixXd &data);


    //=========================================================================================================
    /**
    * Applying the Minimum Energy Combination approach in order to get the signal energy in Y detected by the
    * reference signyl X.
    *
    * @param [in]   Y           measured signal.
    * @param [in]   X           reference signal.
    *
    * @return       signal energy of the reference signal in the measured signal.
    *
    */
    double MEC(MatrixXd &Y, MatrixXd &X);

    //=========================================================================================================
    /**
    * Applying Canoncial Correlation Analysis to get the correlation between the sets of signals of X and Y
    *
    * @param [in]   Y           measured signal.
    * @param [in]   X           reference signal.
    *
    * @return       maximal correlation between the signals.
    *
    */
    double CCA(MatrixXd &Y, MatrixXd &X);

    //=========================================================================================================
    /**
    * The starting point for the thread. After calling start(), the newly created thread calls this function.
    * Returning from this method will end the execution of the thread.
    * Pure virtual method inherited by QThread.
    */
    virtual void run();

    //=========================================================================================================
    /**
    * executing the ssvep BCI algorithm on sensor level
    *
    */
    void ssvepBCIOnSensor();

    //=========================================================================================================
    /**
    * executing the ssvep BCI algorithm on source level
    *
    */
    void ssvepBCIOnSource();

public slots:
    void removePowerLine(bool removePowerLine);
    void setPowerLine(int powerLine);
    void setFeatureExtractionMethod(bool useMEC);
    void setThresholdValues(MyQList thresholds);
    void setChangeSSVEPParameterFlag();
    void setNumClassHits(int NumClassHits);

signals:
    void SSVEPprob(MyQList ssvepProb);
    void classificationResult(double classResult);
    void getFrequencyLabels(MyQList frequencyList);

private:
    QAction*                                            m_pActionBCIConfiguration;          /**< start configuration feature */
    QAction*                                            m_pActionSetupStimulus;             /**< starts stimulus feature */

    QSharedPointer<ssvepBCIConfigurationWidget>         m_pssvepBCIConfigurationWidget;     /**< hold pointer to Widget for BCI configuration */
    QSharedPointer<ssvepBCISetupStimulusWidget>         m_pssvepBCISetupStimulusWidget;     /**< Widget for stimulus setup */

    PluginInputData<NewRealTimeMultiSampleArray>::SPtr  m_pRTMSAInput;          /**< The RealTimeMultiSampleArray input.*/
    PluginInputData<RealTimeSourceEstimate>::SPtr       m_pRTSEInput;           /**< The RealTimeSourceEstimate input.*/

    CircularMatrixBuffer<double>::SPtr                  m_pBCIBuffer_Sensor;    /**< Holds incoming sensor level data.*/
    CircularMatrixBuffer<double>::SPtr                  m_pBCIBuffer_Source;    /**< Holds incoming source level data.*/

    // processing parameter
    bool                    m_bUseSensorData;                   /**< GUI input: Use sensor data stream. */
    ofstream                m_outStreamDebug;                   /**< Outputstream to generate debug file.*/
    bool                    m_bIsRunning;                       /**< Whether BCI is running.*/
    QString                 m_qStringResourcePath;              /**< The path to the BCI resource directory.*/
    bool                    m_bProcessData;                     /**< Whether BCI is to get data out of the continous input data stream, i.e. the EEG data from sensor level.*/
    QMutex                  m_qMutex;                           /**< QMutex to guarantee thread safety.*/

    // adaptable sliding time window with downsampling function
    MatrixXd                m_matSlidingTimeWindow;             /**< Sensor Level: adaptational sliding time window. */
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
    int                     m_iWindowSize;

    // SSVEP parameter
    QList<int>              m_lElectrodeNumbers;                /**< Sensor level: numbers of chosen electrode channels */
    QList<double>           m_lDesFrequencies;                  /**< contains desired frequencies */
    QList<double>           m_lAllFrequencies;                  /**< contains desired frequencies and reference frequencies */
    int                     m_iNumberOfHarmonics;               /**< number of harmonics which will be searched for */
    double                  m_dAlpha;                           /**< parameter for softmax function */
    QList<double>           m_lThresholdValues;                 /**< threshold value for normalized energy probabilities */
    bool                    m_bRemovePowerLine;                 /**< Flag for removing 50 Hz power line signal */
    bool                    m_bUseMEC;                          /**< flag for feature extractiong. If true: use MEC; If false: use CCA */
    QList<int>              m_lIndexOfClassResultSensor;        /**< Sensor level: Classification results on sensor level. */
    int                     m_iPowerLine;                       /**< frequency of the power line [Hz] */
    bool                    m_bChangeSSVEPParameterFlag;        /**< flag for chaning SSVEP parameter */
    int                     m_iNumberOfClassHits;               /**< Number of required classifiaction hits, before a classifiaction is confirmed */
    int                     m_iMaxClassListSize;                /**< maximum size of m_lIndexOfClassResultSensor before it gets deleted */

    // Sensor level
    FiffInfo::SPtr          m_pFiffInfo_Sensor;                 /**< Sensor level: Fiff information for sensor data. */
    QStringList             m_slChosenChannelsSensor;           /**< Sensor level: Features used to calculate data points in feature space on sensor level. */
    QMap<QString, int>      m_mapElectrodePinningScheme;        /**< Sensor level: Loaded pinning scheme of the Duke 64 Dry EEG cap. */

    // Source level
    QVector< VectorXd >     m_vLoadedSourceBoundary;            /**< Source level: Loaded decision boundary on source level. */
    QStringList             m_slChosenChannelsSource;           /**< Source level: Features used to calculate data points in feature space on source level. */
    QMap<QString, int>      m_mapDestrieuxAtlasRegions;         /**< Source level: Loaded Destrieux atlas regions. */
    bool                    m_bInitializeSource;                /**< Source level: initalizie parameter for processing on source level */

    // output
    PluginOutputData<NewRealTimeSampleArray>::SPtr      m_pBCIOutputOne;        /**< The first RealTimeSampleArray of the BCI output.*/
    PluginOutputData<NewRealTimeSampleArray>::SPtr      m_pBCIOutputTwo;        /**< The second RealTimeSampleArray of the BCI output.*/
    PluginOutputData<NewRealTimeSampleArray>::SPtr      m_pBCIOutputThree;      /**< The third RealTimeSampleArray of the BCI output.*/
    PluginOutputData<NewRealTimeSampleArray>::SPtr      m_pBCIOutputFour;       /**< The fourth RealTimeSampleArray of the BCI output.*/
    PluginOutputData<NewRealTimeSampleArray>::SPtr      m_pBCIOutputFive;       /**< The fifth RealTimeSampleArray of the BCI output.*/

    //=========================================================================================================
    /**
    * reading actual segment from the sliding time window and write it to a data Matrix
    *
    * @param [out] data      data space where current data from the sliding time window will be written to
    */
    void readFromSlidingTimeWindow(MatrixXd &data);

    //=========================================================================================================
    /**
    * updates the parameter of the classifiaction process and resets the time window
    */
    void changeSSVEPParameter();

    //=========================================================================================================
    /**
    * updates the list of desired frequencies and of all frequencies which will be examined
    *
    * @param [in]   frequencyList           list of new frequencies
    *
    */
    void setFrequencyList(QList<double> frequencyList);

};

} // NAMESPACE

#endif // SSVEPBCI_H
