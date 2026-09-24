//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     covariance.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Contains the declaration of the Covariance class.
 */

#ifndef COVARIANCE_H
#define COVARIANCE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "covariance_global.h"

#include <scShared/Plugins/abstractalgorithm.h>
#include <utils/generics/circularbuffer.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector>

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

//=============================================================================================================
// DEFINE NAMESPACE COVARIANCEPLUGIN
//=============================================================================================================

namespace COVARIANCEPLUGIN
{

//=============================================================================================================
// COVARIANCEPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * DECLARE CLASS Covariance
 *
 * @brief The Covariance class provides a Covariance algorithm structure.
 */
class COVARIANCESHARED_EXPORT Covariance : public SCSHAREDLIB::AbstractAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "covariance.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SCSHAREDLIB::AbstractAlgorithm)

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
    virtual void init() override;

    //=========================================================================================================
    /**
     * Inits widgets which are used to control this plugin, then emits them in form of a QList.
     */
    void initPluginControlWidgets();

    //=========================================================================================================
    /**
     * Is called when plugin is detached of the stage. Can be used to safe settings.
     */
    virtual void unload() override;

    //=========================================================================================================
    /**
     * Clone the plugin
     */
    virtual QSharedPointer<SCSHAREDLIB::AbstractPlugin> clone() const override;

    virtual bool start() override;
    virtual bool stop() override;

    virtual SCSHAREDLIB::AbstractPlugin::PluginType getType() const override;
    virtual QString getName() const override;

    virtual QWidget* setupWidget() override;

    void update(SCMEASLIB::Measurement::SPtr pMeasurement);

    void showCovarianceWidget();

    void changeSamples(qint32 samples);

    virtual QString getBuildInfo() override;
    virtual QVariantMap getAttributes() const override;
    virtual void setAttributes(const QVariantMap& attributes) override;

protected:
    virtual void run() override;

private:
    QMutex      m_mutex;
    qint32      m_iEstimationSamples;

    UTILSLIB::CircularBuffer_Matrix_double::SPtr        m_pCircularBuffer;              /**< Matrix data circular buffer. */

    QSharedPointer<FIFFLIB::FiffInfo>                   m_pFiffInfo;                    /**< Fiff measurement info.*/

    QSharedPointer<SCSHAREDLIB::PluginInputData<SCMEASLIB::RealTimeMultiSampleArray> >  m_pCovarianceInput;     /**< The RealTimeMultiSampleArray of the Covariance input.*/
    QSharedPointer<SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeCov> >              m_pCovarianceOutput;    /**< The RealTimeCov of the Covariance output.*/

signals:
};
} // NAMESPACE

#endif // COVARIANCE_H
