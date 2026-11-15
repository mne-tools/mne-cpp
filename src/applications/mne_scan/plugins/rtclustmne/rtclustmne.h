//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     rtclustmne.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Contains the declaration of the RtClustMne class.
 */

#ifndef RTCLUSTMNE_H
#define RTCLUSTMNE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtclustmne_global.h"
#include <memory>

#include <scShared/Plugins/abstractalgorithm.h>

#include <base/circular_buffer.h>

#include <Eigen/Core>

#include <fiff/fiff_evoked.h>

#include <inv/inv_source_estimate.h>

#include <mne/mne_inverse_operator.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QFuture>
#include <QPointer>
#include <QSharedPointer>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne::disp
{
class MinimumNormSettingsView;
}

namespace mne
{
class MNEForwardSolution;
class MNEInverseOperator;
}

namespace mne
{
class FiffInfo;
class FiffInfoBase;
}

namespace mne
{
class InvMinimumNorm;
}

namespace mne
{
class RtInvOp;
}

namespace mne
{
class FsAnnotationSet;
class FsSurfaceSet;
}

namespace mne::scan
{
class RealTimeEvokedSet;
class RealTimeMultiSampleArray;
class RealTimeCov;
class RealTimeSourceEstimate;
class RealTimeFwdSolution;
}

//=============================================================================================================
// DEFINE NAMESPACE RTCLUSTMNEPLUGIN
//=============================================================================================================

namespace RTCLUSTMNEPLUGIN
{

//=============================================================================================================
// RTCLUSTMNEPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * DECLARE CLASS RtClustMne
 *
 * @brief The RtClustMne class provides a plugin for estimating distributed source localization in real-time.
 */
class RTCLUSTMNESHARED_EXPORT RtClustMne : public mne::scan::AbstractAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "rtclustmne.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(mne::scan::AbstractAlgorithm)

    friend class RtClustMneSetupWidget;

public:
    //=========================================================================================================
    /**
     * Constructs a RtClustMne.
     */
    RtClustMne();

    //=========================================================================================================
    /**
     * Destroys the RtClustMne.
     */
    ~RtClustMne();

    //=========================================================================================================
    /**
     * AbstractAlgorithm functions
     */
    virtual QSharedPointer<mne::scan::AbstractPlugin> clone() const override;
    virtual void init() override;

    //=========================================================================================================
    /**
     * Inits widgets which are used to control this plugin, then emits them in form of a QList.
     */
    void initPluginControlWidgets();

    virtual void unload() override;
    virtual bool start() override;
    virtual bool stop() override;
    virtual mne::scan::AbstractPlugin::PluginType getType() const override;
    virtual QString getName() const override;
    virtual QWidget* setupWidget() override;
    virtual QString getBuildInfo() override;
    virtual QVariantMap getAttributes() const override;
    virtual void setAttributes(const QVariantMap& attributes) override;

    //=========================================================================================================
    /**
     * Slot called when the fiff info is to be calculated.
     */
    bool calcFiffInfo();

    //=========================================================================================================
    /**
     * Slot to update the real time multi sample array data
     */
    void updateRTMSA(mne::scan::Measurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
     * Slot to update the fiff covariance
     */
    void updateRTC(mne::scan::Measurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
     * Slot to update the fiff evoked
     *
     * @param[in] pMeasurement   The evoked to be appended.
     */
    void updateRTE(mne::scan::Measurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
     * Slot to update the inverse operator
     *
     * @param[in] invOp    The inverse operator to update.
     */
    void updateInvOp(const mne::MNEInverseOperator& invOp);

    //=========================================================================================================
    /**
     * Slot to update the real time forward solution
     */
    void updateRTFS(mne::scan::Measurement::SPtr pMeasurement);

protected:
    //=========================================================================================================
    /**
     * Slot called when the method changed.
     *
     * @param[in] method        The new method.
     */
    void onMethodChanged(const QString& method);

    //=========================================================================================================
    /**
     * Slot called when the CMNE model checkpoint path changed.
     *
     * @param[in] sPath  New absolute path to the model checkpoint file.
     */
    void onModelCheckpointChanged(const QString& sPath);

    //=========================================================================================================
    /**
     * Slot called when the trigger type changed.
     *
     * @param[in] triggerType        The new trigger type.
     */
    void onTriggerTypeChanged(const QString& triggerType);

    //=========================================================================================================
    /**
     * Slot called when the time point changes.
     *
     * @param[in] iTimePointMs        The new time point in ms.
     */
    void onTimePointValueChanged(int iTimePointMs);

    virtual void run() override;

    //=========================================================================================================
    /**
     * Run the CMNE (Contextual MNE) inverse on a single evoked epoch.
     *
     * @param[in] evoked            The evoked epoch to localize.
     * @param[in] fLambda2          Tikhonov regularisation (SNR^-2).
     * @param[in] sModelCheckpoint  Absolute path to the LSTM .onnx checkpoint.
     *
     * @return The CMNE-corrected source estimate, or an empty estimate if prerequisites are missing.
     */
    mne::InvSourceEstimate computeCmneInverse(const mne::FiffEvoked& evoked,
                                              float fLambda2,
                                              const QString& sModelCheckpoint);

    QSharedPointer<mne::scan::PluginInputData<mne::scan::RealTimeFwdSolution>> m_pRTFSInput;       /**< The RealTimeFwdSolution input.*/
    QSharedPointer<mne::scan::PluginInputData<mne::scan::RealTimeMultiSampleArray>> m_pRTMSAInput; /**< The RealTimeMultiSampleArray input.*/
    QSharedPointer<mne::scan::PluginInputData<mne::scan::RealTimeEvokedSet>> m_pRTESInput;         /**< The RealTimeEvoked input.*/
    QSharedPointer<mne::scan::PluginInputData<mne::scan::RealTimeCov>> m_pRTCInput;                /**< The RealTimeCov input.*/
    QSharedPointer<mne::scan::PluginOutputData<mne::scan::RealTimeSourceEstimate>> m_pRTSEOutput;  /**< The RealTimeSourceEstimate output.*/
    std::shared_ptr<mne::CircularBuffer<Eigen::MatrixXd>> m_pCircularMatrixBuffer;                 /**< Holds incoming RealTimeMultiSampleArray data.*/
    std::shared_ptr<mne::CircularBuffer<mne::FiffEvoked>> m_pCircularEvokedBuffer;                 /**< Holds incoming RealTimeMultiSampleArray data.*/
    std::shared_ptr<mne::RtInvOp> m_pRtInvOp;                                                      /**< Real-time inverse operator. */
    std::shared_ptr<mne::MNEForwardSolution> m_pFwd;                                               /**< Forward solution. */
    std::shared_ptr<mne::FiffCov> m_pNoiseCov;                                                     /**< Noise Covariance Matrix. */
    std::shared_ptr<mne::FsAnnotationSet> m_pAnnotationSet;                                        /**< FsAnnotation set. */
    std::shared_ptr<mne::FsSurfaceSet> m_pSurfaceSet;                                              /**< FsSurface set. */
    std::shared_ptr<mne::FiffInfoBase> m_pFiffInfoForward;                                         /**< Fiff information of the forward solution. */
    std::shared_ptr<mne::FiffInfo> m_pFiffInfo;                                                    /**< Fiff information. */
    std::shared_ptr<mne::FiffInfo> m_pFiffInfoInput;                                               /**< Fiff information of the evoked. */

    bool m_bEvokedInput;       /**< Flag whether an evoked input was received. */
    bool m_bRawInput;          /**< Flag whether a raw data input was received. */
    bool m_bUpdateMinimumNorm; /**< Flag whether to update the miniumum norm object. */

    QMutex m_qMutex;        /**< The mutex ensuring thread safety. */
    QFuture<void> m_future; /**< The future monitoring the clustering. */

    mne::FiffEvoked m_currentEvoked;
    mne::FiffCoordTrans m_mriHeadTrans; /**< the Mri Head transformation. */

    qint32 m_iNumAverages;  /**< The number of trials/averages to store. */
    qint32 m_iDownSample;   /**< Down sample factor. */
    qint32 m_iTimePointSps; /**< The time point to pick from the data in samples. */

    QString m_sAtlasDir;        /**< File to Atlas. */
    QString m_sSurfaceDir;      /**< File to FsSurface. */
    QString m_sAvrType;         /**< The average type. */
    QString m_sMethod;          /**< The method: "MNE" | "dSPM" | "sLORETA" | "CMNE". */
    QString m_sModelCheckpoint; /**< CMNE LSTM model checkpoint path (.onnx). */
    QFile m_fMriHeadTrans;      /**< The Head - Mri transformation. */

    std::vector<std::string> m_qListCovChNames;   /**< Covariance channel names. */
    std::vector<std::string> m_qListPickChannels; /**< Channels to pick. */

    mne::MNEInverseOperator m_invOp; /**< The inverse operator. */

signals:
    void responsibleTriggerTypesChanged(const QStringList& lResponsibleTriggerTypes);
};
} // NAMESPACE

#endif // RTCLUSTMNE_H
