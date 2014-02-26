//=============================================================================================================
/**
* @file     bci.h
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     December, 2013
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

#include <mne_x/Interfaces/IAlgorithm.h>

#include <generics/circularmatrixbuffer.h>

#include <xMeas/newrealtimesamplearray.h>
#include <xMeas/newrealtimemultisamplearray.h>
#include <xMeas/realtimesourceestimate.h>

//*************************************************************************************************************
//=============================================================================================================
// QT STL INCLUDES
//=============================================================================================================

#include <QtWidgets>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE TMSIPlugin
//=============================================================================================================

namespace BCIPlugin
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEX;
using namespace XMEASLIB;
using namespace IOBuffer;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* BCI...
*
* @brief The BCI class provides an EEG BCI.
*/
class BCISHARED_EXPORT BCI : public IAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "mne_x/1.0" FILE "bci.json") //NEw Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(MNEX::IAlgorithm)

    friend class BCISetupWidget;

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

    void updateSensor(XMEASLIB::NewMeasurement::SPtr pMeasurement);
    void updateSource(XMEASLIB::NewMeasurement::SPtr pMeasurement);

protected:
    //=========================================================================================================
    /**
    * The starting point for the thread. After calling start(), the newly created thread calls this function.
    * Returning from this method will end the execution of the thread.
    * Pure virtual method inherited by QThread.
    */
    virtual void run();

private:
    PluginOutputData<NewRealTimeSampleArray>::SPtr      m_pBCIOutput;           /**< The RealTimeSampleArray of the BCI output.*/

    PluginInputData<NewRealTimeMultiSampleArray>::SPtr  m_pRTMSAInput;          /**< The RealTimeMultiSampleArray input.*/
    PluginInputData<RealTimeSourceEstimate>::SPtr       m_pRTSEInput;           /**< The RealTimeSourceEstimate input.*/

    CircularMatrixBuffer<double>::SPtr                  m_pBCIBuffer_Sensor;    /**< Holds incoming sensor level data.*/
    CircularMatrixBuffer<double>::SPtr                  m_pBCIBuffer_Source;    /**< Holds incoming source level data.*/

    bool                m_bIsRunning;                   /**< Whether BCI is running.*/
    bool                m_bProcessData;                 /**< Whether BCI is to get data out of the continous input data stream, i.e. the EEG data from sensor level.*/
    QString             m_qStringResourcePath;          /**< The path to the BCI resource directory.*/
    QMutex              m_qMutex;

    FiffInfo::SPtr      m_pFiffInfo_Sensor;             /**< Fiff information for sensor data. */
    MatrixXd            m_mSlidingWindowSensor;         /**< Working (sliding) matrix, used to store data for feature calculation on sensor level. */
    int                 m_iCurrentIndexSensor;          /**< Index of the amount of data which was already filled in the sliding window. */

    bool                m_bUseSensorData;               /**< GUI input: Use sensor data stream. */
    bool                m_bUseSourceData;               /**< GUI input: Use source data stream. */
    double              m_dSlidingWindowSize;           /**< GUI input: Size of the sliding window in s. */
    double              m_dBaseLineWindowSize;          /**< GUI input: Size of the baseline window in s. */
    int                 m_iNumberSubSignals;            /**< GUI input: Number of subsignals. */
    QString             m_sSensorBoundaryPath;          /**< GUI input: Input path for boundary file on sensor level. */
    QString             m_sSourceBoundaryPath;          /**< GUI input: Input path for boundary file on source level. */

    QVector<double>     m_qVLoadedSensorBoundary;       /**< Loaded decision boundary on sensor level. */
    QVector<double>     m_qVLoadedSourceBoundary;       /**< Loaded decision boundary on source level. */

};

} // NAMESPACE

#endif // BCI_H
