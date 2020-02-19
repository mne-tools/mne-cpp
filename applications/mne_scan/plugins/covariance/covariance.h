//=============================================================================================================
/**
 * @file     covariance.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
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

#include <scShared/Interfaces/IAlgorithm.h>


//*************************************************************************************************************
//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB {
    class FiffCov;
    class FiffInfo;
}

namespace RTPROCESSINGLIB {
    class RtCov;
}

namespace SCMEASLIB {
    class RealTimeMultiSampleArray;
    class RealTimeCov;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE COVARIANCEPLUGIN
//=============================================================================================================

namespace COVARIANCEPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// COVARIANCEPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

class CovarianceSettingsWidget;


//=============================================================================================================
/**
 * DECLARE CLASS Covariance
 *
 * @brief The Covariance class provides a Covariance algorithm structure.
 */
class COVARIANCESHARED_EXPORT Covariance : public SCSHAREDLIB::IAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "covariance.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::IAlgorithm)

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
    virtual QSharedPointer<SCSHAREDLIB::IPlugin> clone() const;

    virtual bool start();
    virtual bool stop();

    virtual SCSHAREDLIB::IPlugin::PluginType getType() const;
    virtual QString getName() const;

    virtual QWidget* setupWidget();

    void update(SCMEASLIB::Measurement::SPtr pMeasurement);

    void appendCovariance(const FIFFLIB::FiffCov &p_pCovariance);

    void showCovarianceWidget();

    void changeSamples(qint32 samples);

protected:
    virtual void run();

private:
    QMutex      mutex;

    bool        m_bIsRunning;                       /**< If thread is running */
    bool        m_bProcessData;                     /**< If data should be received for processing */

    qint32      m_iEstimationSamples;

    QVector<FIFFLIB::FiffCov>                       m_qVecCovData;                  /**< Covariance data set */

    QAction*                                        m_pActionShowAdjustment;

    QSharedPointer<FIFFLIB::FiffInfo>               m_pFiffInfo;                    /**< Fiff measurement info.*/
    QSharedPointer<RTPROCESSINGLIB::RtCov>          m_pRtCov;                       /**< Real-time covariance. */

    QSharedPointer<CovarianceSettingsWidget>        m_pCovarianceWidget;

    QSharedPointer<SCSHAREDLIB::PluginInputData<SCMEASLIB::RealTimeMultiSampleArray> >  m_pCovarianceInput;     /**< The RealTimeMultiSampleArray of the Covariance input.*/
    QSharedPointer<SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeCov> >              m_pCovarianceOutput;    /**< The RealTimeCov of the Covariance output.*/

signals:

};

} // NAMESPACE

#endif // COVARIANCE_H
