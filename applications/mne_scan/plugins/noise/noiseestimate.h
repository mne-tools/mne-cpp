//=============================================================================================================
/**
 * @file     noiseestimate.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     July, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the declaration of the NoiseEstimate class.
 *
 */

#ifndef NOISEESTIMATE_H
#define NOISEESTIMATE_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "noiseestimate_global.h"

#include <scShared/Interfaces/IAlgorithm.h>
#include <utils/generics/circularmatrixbuffer.h>
#include <scMeas/realtimemultisamplearray.h>
#include <scMeas/realtimespectrum.h>
#include <rtprocessing/rtnoise.h>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/SparseCore>
#include <unsupported/Eigen/FFT>

//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include <fiff/fiff_info.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>

#include <QVector>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE NOISEESTIMATEPLUGIN
//=============================================================================================================

namespace NOISEESTIMATEPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace IOBUFFER;
using namespace RTPROCESSINGLIB;

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
 * DECLARE CLASS RTHPI
 *
 * @brief The NoiseEstimate class provides a NoiseEstimate algorithm structure.
 */
class NOISE_ESTIMATESHARED_EXPORT NoiseEstimate : public IAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "noise.json") //NEW Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::IAlgorithm)

    friend class NoiseEstimateSetupWidget;

public:
    //=========================================================================================================
    /**
    * Constructs a RtHpi.
    */
    NoiseEstimate();

    //=========================================================================================================
    /**
    * Destroys the RtHpi.
    */
    ~NoiseEstimate();

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
    * Clone the plugin
    */
    virtual QSharedPointer<IPlugin> clone() const;

    virtual bool start();
    virtual bool stop();

    virtual IPlugin::PluginType getType() const;
    virtual QString getName() const;

    virtual QWidget* setupWidget();

    void update(SCMEASLIB::Measurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
    * Add the spectrum result into a list
    */
    void appendNoiseSpectrum(Eigen::MatrixXd);

signals:
    //=========================================================================================================
    /**
    * Emitted when fiffInfo is available
    */
    void fiffInfoAvailable();
    //=========================================================================================================
    /**
    * Emitted Noise parameters
    */
    void SetNoisePara(qint32 nFFT, int fs);

protected:
    virtual void run();

private:
    //=========================================================================================================
    /**
    * Initialises the output connector.
    */
    void initConnector();

    PluginInputData<RealTimeMultiSampleArray>::SPtr   m_pRTMSAInput;     /**< The RealTimeMultiSampleArray of the noise plugin input.*/
    PluginOutputData<RealTimeSpectrum>::SPtr  m_pFSOutput;                   /**< The NE of the noise plugin output.*/


    FiffInfo::SPtr  m_pFiffInfo;                        /**< Fiff measurement info.*/

    CircularMatrixBuffer<double>::SPtr   m_pBuffer;     /**< Holds incoming data.*/

    RtNoise::SPtr m_pRtNoise;                       /**< Real-time Noise Estimation. */
    //RtNoise * m_pRtNoise;                       /**< Real-time Noise Estimation. */

    QVector<MatrixXd>   m_qVecSpecData;     /**< Evoked data set */

    bool m_bIsRunning;      /**< If source lab is running */
    bool m_bProcessData;    /**< If data should be received for processing */

    double m_Fs;            /**< sample rate */
    qint32 m_iFFTlength;    /**< number of bins for FFT */
    float m_DataLen;        /**< the length of data used for spectrum calculation */
    qint8 m_x_scale_type;   /**< Type of x-axis scale: normal (0) or log (1) */

    QMutex m_qMutex;       /**< mutex for spectrum */

};

} // NAMESPACE

#endif // NOISEESTIMATE_H
