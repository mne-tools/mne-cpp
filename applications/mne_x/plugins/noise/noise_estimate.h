//=============================================================================================================
/**
* @file     noise_estimate.h
* @author   Limin Sun <liminsun@nmr.mgh.harvard.edu>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Limin Sun, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the RTHPI class.
*
*/

#ifndef NOISE_ESTIMATE_H
#define NOISE_ESTIMATE_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "noise_estimate_global.h"

#include <mne_x/Interfaces/IAlgorithm.h>
#include <generics/circularmatrixbuffer.h>
#include <xMeas/newrealtimemultisamplearray.h>
#include <xMeas/noiseestimation.h>

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
// DEFINE NAMESPACE RtHpiPlugin
//=============================================================================================================

namespace NoiseEstimatePlugin
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
* DECLARE CLASS RTHPI
*
* @brief The NoiseEstimate class provides a NoiseEstimate algorithm structure.
*/
//class DUMMYTOOLBOXSHARED_EXPORT DummyToolbox : public IAlgorithm
class NOISE_ESTIMATESHARED_EXPORT NoiseEstimate : public IAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "mne_x/1.0" FILE "noise.json") //NEW Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(MNEX::IAlgorithm)

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
    void init();

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

    void update(XMEASLIB::NewMeasurement::SPtr pMeasurement);

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
    //=========================================================================================================
    /**
    * Emitted data to plot
    */
    //void RePlot(MatrixXf psdx);

protected:
    virtual void run();

private:
    //=========================================================================================================
    /**
    * Initialises the output connector.
    */
    void initConnector();

    PluginInputData<NewRealTimeMultiSampleArray>::SPtr   m_pRTMSAInput;     /**< The NewRealTimeMultiSampleArray of the noise plugin input.*/
    PluginOutputData<NoiseEstimation>::SPtr  m_pNEOutput;                   /**< The NE of the noise plugin output.*/


    FiffInfo::SPtr  m_pFiffInfo;                        /**< Fiff measurement info.*/

    CircularMatrixBuffer<double>::SPtr   m_pBuffer;     /**< Holds incoming data.*/

    bool m_bIsRunning;      /**< If source lab is running */
    bool m_bProcessData;    /**< If data should be received for processing */

    double m_Fs;
    qint32 m_iFFTlength;

    //MatrixXd sum_psdx;

protected:
    int NumOfBlocks;
    int BlockSize  ;
    int Sensors    ;
    int BlockIndex ;

    MatrixXd CircBuf;

};

} // NAMESPACE

#endif // NOISE_ESTIMATE_H
