//=============================================================================================================
/**
* @file     covariance.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the Covariance class.
*
*/

#ifndef COVARIANCE_H
#define COVARIANCE_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "covariance_global.h"

#include <mne_x/Interfaces/IAlgorithm.h>
#include <generics/circularmatrixbuffer.h>
#include <xMeas/newrealtimemultisamplearray.h>
#include <xMeas/realtimecov.h>
#include <rtInv/rtcov.h>


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include <fiff/fiff_info.h>
#include <fiff/fiff_cov.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QVector>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE CovariancePlugin
//=============================================================================================================

namespace CovariancePlugin
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEX;
using namespace XMEASLIB;
using namespace IOBuffer;
using namespace RTINVLIB;
using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class CovarianceSettingsWidget;


//=============================================================================================================
/**
* DECLARE CLASS Covariance
*
* @brief The Covariance class provides a Covariance algorithm structure.
*/
class COVARIANCESHARED_EXPORT Covariance : public IAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "mne_x/1.0" FILE "covariance.json") //NEw Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(MNEX::IAlgorithm)

    friend class CovarianceSettingsWidget;

public:
    //=========================================================================================================
    /**
    * Constructs a Covariance.
    */
    Covariance();

    //=========================================================================================================
    /**
    * Destroys the Covariance.
    */
    ~Covariance();

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

    void update(XMEASLIB::NewMeasurement::SPtr pMeasurement);

    void appendCovariance(FiffCov::SPtr p_pCovariance);

    void showCovarianceWidget();

    void changeSamples(qint32 samples);

signals:
    //=========================================================================================================
    /**
    * Emitted when fiffInfo is available
    */
    void fiffInfoAvailable();

protected:
    virtual void run();

private:
    QMutex mutex;

    PluginInputData<NewRealTimeMultiSampleArray>::SPtr  m_pCovarianceInput;     /**< The NewRealTimeMultiSampleArray of the Covariance input.*/
    PluginOutputData<RealTimeCov>::SPtr                 m_pCovarianceOutput;    /**< The RealTimeCov of the Covariance output.*/

    FiffInfo::SPtr  m_pFiffInfo;                                /**< Fiff measurement info.*/

    CircularMatrixBuffer<double>::SPtr   m_pCovarianceBuffer;   /**< Holds incoming data.*/

    RtCov::SPtr m_pRtCov;                       /**< Real-time covariance. */

    QVector<FiffCov::SPtr>   m_qVecCovData;     /**< Covariance data set */

    bool m_bIsRunning;                          /**< If source lab is running */
    bool m_bProcessData;                        /**< If data should be received for processing */

    qint32 m_iEstimationSamples;

    QSharedPointer<CovarianceSettingsWidget> m_pCovarianceWidget;

    QAction* m_pActionShowAdjustment;
};

} // NAMESPACE

#endif // COVARIANCE_H
