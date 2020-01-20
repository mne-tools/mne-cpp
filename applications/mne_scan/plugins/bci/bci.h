//=============================================================================================================
/**
 * @file     bci.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @version  dev
 * @date     December, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch, Viktor Klueber. All rights reserved.
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
 * @brief    Contains the declaration of the BCI class.
 *
 */

#ifndef BCI_H
#define BCI_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bci_global.h"

#include <utils/generics/circularmatrixbuffer.h>
#include <scShared/Interfaces/IAlgorithm.h>
#include <scMeas/realtimesamplearray.h>
#include <scMeas/realtimemultisamplearray.h>
#include <scMeas/realtimesourceestimate.h>

#include <utils/filterTools/filterdata.h>

#include <fstream>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QtConcurrent/QtConcurrent>

#include "FormFiles/bcisetupwidget.h"
#include "FormFiles/bcifeaturewindow.h"


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE BCIPLUGIN
//=============================================================================================================

namespace BCIPLUGIN
{

//*************************************************************************************************************
//=============================================================================================================
// TypeDefs
//=============================================================================================================

typedef QList< QList<double> > MyQList;


//=============================================================================================================
/**
 * BCI...
 *
 * @brief The BCI class provides an EEG BCI.
 */
class BCISHARED_EXPORT BCI : public SCSHAREDLIB::IAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "bci.json") //NEw Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::IAlgorithm)

    friend class BCISetupWidget;
    friend class BCIFeatureWindow;

public:
    //=========================================================================================================
    /**
    * Constructs a BCI.
    */
    BCI();

    //=========================================================================================================
    /**
    * Destroys the BCI.
    */
    virtual ~BCI();

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
    * Calculates the filtered signal of chdata
    *
    * @param [in] rowdata QPair with number of the row and the data samples as a RowVectorXd.
    */
    void applyMeanCorrectionConcurrently(QPair<int,RowVectorXd>& rowdata);

    //=========================================================================================================
    /**
    * Calculates the filtered signal of chdata
    *
    * @param [in] chdata QPair with number of the row and the data samples as a RowVectorXd.
    */
    void applyFilterOperatorConcurrently(QPair<int,RowVectorXd> &chdata);

    //=========================================================================================================
    /**
    * Calculates the features on sensor level
    *
    * @param [in] chdata QPair with number of the row and the data samples as a RowVectorXd.
    * @param [out] QPair<int,QList<double>> calculated features.
    */
    QPair< int,QList<double> > applyFeatureCalcConcurrentlyOnSensorLevel(const QPair<int,RowVectorXd> &chdata);

    //=========================================================================================================
    /**
    * Classifies the features on sensor level
    *
    * @param [in] featData QList<double> holds the feature data point (i.e. 2 electrodes make this parameter have size of 2).
    * @param [out] double calculated classification value.
    */
    double applyClassificationCalcConcurrentlyOnSensorLevel(QList<double> &featData);

    //=========================================================================================================
    /**
    * Calculates the function value of the decision function (boundary) for a given feature point
    *
    * @param [in] featData QList<double> holds the feature data point (i.e. 2 electrodes make this parameter have size of 2).
    * @param [out] double function value.
    */
    double classificationBoundaryValue(const QList<double> &featData);

    //=========================================================================================================
    /**
    * Clears features
    *
    */
    void clearFeatures();

    //=========================================================================================================
    /**
    * Clears all classification results
    *
    */
    void clearClassifications();

    //=========================================================================================================
    /**
    * Check for artefact in data
    *
    */
    bool hasThresholdArtefact(const QList<QPair<int, RowVectorXd> > &data);

    //=========================================================================================================
    /**
    * Look for trigger in stim channel
    *
    */
    bool lookForTrigger(const MatrixXd &data);

    //=========================================================================================================
    /**
    * The starting point for the thread. After calling start(), the newly created thread calls this function.
    * Returning from this method will end the execution of the thread.
    * Pure virtual method inherited by QThread.
    */
    virtual void run();

    //=========================================================================================================
    /**
    * Do BCI stuff with data received from sensor level
    *
    */
    void BCIOnSensorLevel();

    //=========================================================================================================
    /**
    * Do BCI stuff with data received from source level
    *
    */
    void BCIOnSourceLevel();

signals:
    void paintFeatures(MyQList features, bool bTrigerActivated);

private:
    SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeSampleArray>::SPtr      m_pBCIOutputOne;        /**< The first RealTimeSampleArray of the BCI output.*/
    SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeSampleArray>::SPtr      m_pBCIOutputTwo;        /**< The second RealTimeSampleArray of the BCI output.*/
    SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeSampleArray>::SPtr      m_pBCIOutputThree;      /**< The third RealTimeSampleArray of the BCI output.*/
    SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeSampleArray>::SPtr      m_pBCIOutputFour;       /**< The fourth RealTimeSampleArray of the BCI output.*/
    SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeSampleArray>::SPtr      m_pBCIOutputFive;       /**< The fifth RealTimeSampleArray of the BCI output.*/

    SCSHAREDLIB::PluginInputData<SCMEASLIB::RealTimeMultiSampleArray>::SPtr  m_pRTMSAInput;          /**< The RealTimeMultiSampleArray input.*/
    SCSHAREDLIB::PluginInputData<SCMEASLIB::RealTimeSourceEstimate>::SPtr    m_pRTSEInput;           /**< The RealTimeSourceEstimate input.*/

    IOBUFFER::CircularMatrixBuffer<double>::SPtr                  m_pBCIBuffer_Sensor;    /**< Holds incoming sensor level data.*/
    IOBUFFER::CircularMatrixBuffer<double>::SPtr                  m_pBCIBuffer_Source;    /**< Holds incoming source level data.*/

    QSharedPointer<UTILSLIB::FilterData>                          m_filterOperator;       /**< Holds filter with specified properties by the user.*/

    QSharedPointer<BCIFeatureWindow>                    m_BCIFeatureWindow;     /**< Holds pointer to BCIFeatureWindow for visualization purposes.*/

    std::ofstream                m_outStreamDebug;                   /**< Outputstream to generate debug file.*/

    bool                    m_bIsRunning;                       /**< Whether BCI is running.*/
    QString                 m_qStringResourcePath;              /**< The path to the BCI resource directory.*/
    bool                    m_bProcessData;                     /**< Whether BCI is to get data out of the continous input data stream, i.e. the EEG data from sensor level.*/
    bool                    m_bTriggerActivated;                /**< Whether the trigger was activated.*/
    QMutex                  m_qMutex;                           /**< QMutex to guarantee thread safety.*/

    // Sensor level
    SCMEASLIB::FiffInfo::SPtr          m_pFiffInfo_Sensor;                 /**< Sensor level: Fiff information for sensor data. */
    bool                    m_bFiffInfoInitialised_Sensor;      /**< Sensor level: Fiff information initialised. */
    bool                    m_bFillSensorWindowFirstTime;       /**< Sensor level: Flag if the working matrix m_mSlidingWindowSensor is being filled for the first time. */
    MatrixXd                m_matSlidingWindowSensor;           /**< Sensor level: Working (sliding) matrix, used to store data for feature calculation on sensor level. */
    MatrixXd                m_matTimeBetweenWindowsSensor;      /**< Sensor level: Samples stored during time between windows on sensor level. */
    int                     m_iTBWIndexSensor;                  /**< Sensor level: Index of the amount of data which was already filled during the time between windows. */
    int                     m_iNumberOfCalculatedFeatures;      /**< Sensor level: Index which is iterated until enough features are calculated and classified to generate a final classifcation result.*/
    QVector< VectorXd >     m_vLoadedSensorBoundary;            /**< Sensor level: Loaded decision boundary on sensor level. */
    QStringList             m_slChosenFeatureSensor;            /**< Sensor level: Features used to calculate data points in feature space on sensor level. */
    QMap<QString, int>      m_mapElectrodePinningScheme;        /**< Sensor level: Loaded pinning scheme of the Duke 128 EEG cap. */
    QList< QPair< int,QList<double> > >  m_lFeaturesSensor;     /**< Sensor level: Features calculated on sensor level. */
    QList<double>           m_lClassResultsSensor;              /**< Sensor level: Classification results on sensor level. */
    MatrixXd                m_matStimChannelSensor;             /**< Sensor level: Stim channel. */
    MatrixXd                m_matTimeBetweenWindowsStimSensor;  /**< Sensor level: Stim channel. */

    // Source level
    QVector< VectorXd >     m_vLoadedSourceBoundary;            /**< Source level: Loaded decision boundary on source level. */
    QStringList             m_slChosenFeatureSource;            /**< Source level: Features used to calculate data points in feature space on source level. */
    QMap<QString, int>      m_mapDestrieuxAtlasRegions;         /**< Source level: Loaded Destrieux atlas regions. */

    // GUI stuff
    bool                    m_bSubtractMean;                    /**< GUI input: Subtract mean from window. */
    bool                    m_bUseFilter;                       /**< GUI input: Use filtering. */
    bool                    m_bUseSensorData;                   /**< GUI input: Use sensor data stream. */
    bool                    m_bUseSourceData;                   /**< GUI input: Use source data stream. */
    bool                    m_bDisplayFeatures;                 /**< GUI input: Display features in feature window. */
    bool                    m_bUseArtefactThresholdReduction;   /**< GUI input: Whether BCI uses a threshold to obmit atrefacts.*/
    double                  m_dSlidingWindowSize;               /**< GUI input: Size of the sliding window in s. */
    double                  m_dTimeBetweenWindows;              /**< GUI input: Time between windows/feature calculation in s. */
    double                  m_dFilterLowerBound;                /**< GUI input: Filter lower bound in Hz. */
    double                  m_dFilterUpperBound;                /**< GUI input: Filter upper bound in Hz. */
    double                  m_dParcksWidth;                     /**< GUI input: Parck filter algorithm width in Hz. */
    double                  m_dThresholdValue;                  /**< GUI input: Threshold in micro volts. */
    double                  m_dDisplayRangeBoundary;            /**< GUI input: Display range for the boundary values. */
    double                  m_dDisplayRangeVariances;           /**< GUI input: Display range for the variance values. */
    double                  m_dDisplayRangeElectrodes;          /**< GUI input: Display range for the electrode time values. */
    int                     m_iFilterOrder;                     /**< GUI input: Filter order. */
    int                     m_iNumberFeatures;                  /**< GUI input: Number of classifactions to store until they get averaged. */
    int                     m_iNumberFeaturesToDisplay;         /**< GUI input: Number of features to display. */
    int                     m_iFeatureCalculationType;          /**< GUI input: Type of feature calculation (variance/log of variance/...). */
};

} // NAMESPACE

#endif // BCI_H
