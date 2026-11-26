//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     rtclustmne.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Definition of the RtClustMne class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/log.h>
#include "rtclustmne.h"

#include "FormFiles/rtclustmnesetupwidget.h"
#include <mne/base/string_utils.h>
#include <mne/viz/backends/qt/viewers/minimumnormsettingsview.h>
#include <mne/fs/fs_annotationset.h>
#include <mne/fs/fs_surfaceset.h>
#include <mne/fiff/fiff_ch_name.h>
#include <mne/fiff/fiff_info.h>
#include <mne/mne/mne_forward_solution.h>
#include <mne/inv/inv_source_estimate.h>
#include <mne/mne/mne_epoch_data_list.h>
#include <mne/dsp/rt/rt_inv_op.h>
#include <mne/inv/minimum_norm/inv_cmne.h>
#include <mne/inv/minimum_norm/inv_cmne_settings.h>
#include <scMeas/realtimesourceestimate.h>
#include <scMeas/realtimemultisamplearray.h>
#include <scMeas/realtimecov.h>
#include <scMeas/realtimeevokedset.h>
#include <scMeas/realtimefwdsolution.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
#include <QDebug>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <memory>
#include <optional>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTCLUSTMNEPLUGIN;
using namespace mne;
using namespace mne::scan;
using namespace mne::viz::qt;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtClustMne::RtClustMne()
: m_pCircularMatrixBuffer(std::make_shared<mne::CircularBuffer<Eigen::MatrixXd>>(40))
, m_pCircularEvokedBuffer(std::make_shared<CircularBuffer<mne::FiffEvoked>>(40))
, m_bEvokedInput(false)
, m_bRawInput(false)
, m_iNumAverages(1)
, m_iDownSample(1)
, m_iTimePointSps(0)
, m_sAtlasDir(QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/subjects/sample/label")
, m_sSurfaceDir(QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/subjects/sample/surf")
, m_sAvrType("3")
, m_sMethod("dSPM")
, m_fMriHeadTrans(QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/all-trans.fif")
{
}

//=============================================================================================================

RtClustMne::~RtClustMne()
{
    if (this->isRunning()) {
        stop();
    }
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> RtClustMne::clone() const
{
    auto pRtClustMneClone = QSharedPointer<RtClustMne>::create();
    return pRtClustMneClone;
}

//=============================================================================================================

void RtClustMne::init()
{
    // Inits
    m_pAnnotationSet = std::make_shared<FsAnnotationSet>((m_sAtlasDir + "/lh.aparc.a2009s.annot").toStdU16String(), (m_sAtlasDir + "/rh.aparc.a2009s.annot").toStdU16String());
    m_pSurfaceSet = std::make_shared<FsSurfaceSet>((m_sSurfaceDir + "/lh.orig").toStdU16String(), (m_sSurfaceDir + "/rh.orig").toStdU16String());
    // The sample-data transform is optional; a missing file leaves no transform
    m_mriHeadTrans = mne::FiffCoordTrans::read(m_fMriHeadTrans.fileName().toStdU16String()).value_or(mne::FiffCoordTrans());

    // Input
    m_pRTMSAInput = PluginInputData<RealTimeMultiSampleArray>::create(this, "MNE RTMSA In", "MNE real-time multi sample array input data");
    connect(m_pRTMSAInput.data(), &PluginInputConnector::notify,
            this, &RtClustMne::updateRTMSA, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTMSAInput);

    m_pRTESInput = PluginInputData<RealTimeEvokedSet>::create(this, "MNE RTE In", "MNE real-time evoked input data");
    connect(m_pRTESInput.data(), &PluginInputConnector::notify,
            this, &RtClustMne::updateRTE, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTESInput);

    m_pRTCInput = PluginInputData<RealTimeCov>::create(this, "MNE RTC In", "MNE real-time covariance input data");
    connect(m_pRTCInput.data(), &PluginInputConnector::notify,
            this, &RtClustMne::updateRTC, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTCInput);

    m_pRTFSInput = PluginInputData<RealTimeFwdSolution>::create(this, "MNE RTFS In", "MNE real-time forward solution input data");
    connect(m_pRTFSInput.data(), &PluginInputConnector::notify,
            this, &RtClustMne::updateRTFS, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTFSInput);

    // Output
    m_pRTSEOutput = PluginOutputData<RealTimeSourceEstimate>::create(this, "MNE Out", "MNE output data");
    m_outputConnectors.append(m_pRTSEOutput);
    m_pRTSEOutput->measurementData()->setName(this->getName()); //Provide name to auto store widget settings

    // Set the annotation and surface data and mri-head transformation
    if (m_pAnnotationSet->size() != 0) {
        m_pRTSEOutput->measurementData()->setAnnotSet(m_pAnnotationSet);
    }

    if (m_pSurfaceSet->size() != 0) {
        m_pRTSEOutput->measurementData()->setSurfSet(m_pSurfaceSet);
    }

    if (!m_mriHeadTrans.empty()) {
        m_pRTSEOutput->measurementData()->setMriHeadTrans(m_mriHeadTrans);
    }
}

//=============================================================================================================

void RtClustMne::initPluginControlWidgets()
{
    std::vector<std::unique_ptr<QWidget>> controlWidgets;

    MinimumNormSettingsView* pMinimumNormSettingsView = makeControlWidget<MinimumNormSettingsView>(controlWidgets, QString("MNESCAN/%1").arg(this->getName()));
    connect(this, &RtClustMne::guiModeChanged,
            pMinimumNormSettingsView, &MinimumNormSettingsView::setGuiMode);
    pMinimumNormSettingsView->setObjectName("group_tab_Settings_Source Localization");

    connect(pMinimumNormSettingsView, &MinimumNormSettingsView::methodChanged,
            this, &RtClustMne::onMethodChanged);
    connect(pMinimumNormSettingsView, &MinimumNormSettingsView::triggerTypeChanged,
            this, &RtClustMne::onTriggerTypeChanged);
    connect(pMinimumNormSettingsView, &MinimumNormSettingsView::timePointChanged,
            this, &RtClustMne::onTimePointValueChanged);
    connect(pMinimumNormSettingsView, &MinimumNormSettingsView::modelCheckpointChanged,
            this, &RtClustMne::onModelCheckpointChanged);
    connect(this, &RtClustMne::responsibleTriggerTypesChanged,
            pMinimumNormSettingsView, &MinimumNormSettingsView::setTriggerTypes);

    // Push any restored checkpoint path to the view so the user sees the persisted value.
    if (!m_sModelCheckpoint.isEmpty()) {
        pMinimumNormSettingsView->setModelCheckpoint(mne::path_from_utf8(m_sModelCheckpoint.toStdString()));
    } else if (!pMinimumNormSettingsView->getModelCheckpoint().empty()) {
        // The view loaded a previously persisted path via its own QSettings.
        m_sModelCheckpoint = QString::fromStdString(mne::utf8_from_path(pMinimumNormSettingsView->getModelCheckpoint()));
    }


    offerControlWidgets(std::move(controlWidgets));

    m_bPluginControlWidgetsInit = true;
}

//=============================================================================================================

void RtClustMne::unload()
{
}

//=============================================================================================================

bool RtClustMne::calcFiffInfo()
{
    QMutexLocker locker(&m_qMutex);

    if (m_qListCovChNames.size() > 0 && m_pFiffInfoInput && m_pFiffInfoForward) {
        Log::debug("[RtClustMne::calcFiffInfoFiff] Infos available");

        //        qDebug() << "RtClustMne::calcFiffInfo - m_qListCovChNames" << m_qListCovChNames;
        //        qDebug() << "RtClustMne::calcFiffInfo - m_pFiffInfoForward->ch_names" << m_pFiffInfoForward->ch_names;
        //        qDebug() << "RtClustMne::calcFiffInfo - m_pFiffInfoInput->ch_names" << m_pFiffInfoInput->ch_names;

        // Align channel names of the forward solution to the incoming averaged (currently acquired) data
        // Find out whether the forward solution depends on only MEG, EEG or both MEG and EEG channels
        QStringList forwardChannelsTypes;
        m_pFiffInfoForward->ch_names.clear();
        int counter = 0;

        for (qint32 x = 0; x < std::ssize(m_pFiffInfoForward->chs); ++x) {
            if (forwardChannelsTypes.contains("MEG") && forwardChannelsTypes.contains("EEG"))
                break;

            if (m_pFiffInfoForward->chs[x].kind == FIFFV_MEG_CH && !forwardChannelsTypes.contains("MEG"))
                forwardChannelsTypes << "MEG";

            if (m_pFiffInfoForward->chs[x].kind == FIFFV_EEG_CH && !forwardChannelsTypes.contains("EEG"))
                forwardChannelsTypes << "EEG";
        }

        //If only MEG channels are used
        if (forwardChannelsTypes.contains("MEG") && !forwardChannelsTypes.contains("EEG")) {
            for (qint32 x = 0; x < std::ssize(m_pFiffInfoInput->chs); ++x) {
                if (m_pFiffInfoInput->chs[x].kind == FIFFV_MEG_CH) {
                    m_pFiffInfoForward->chs[counter].ch_name = m_pFiffInfoInput->chs[x].ch_name;
                    m_pFiffInfoForward->ch_names.push_back(m_pFiffInfoInput->chs[x].ch_name);
                    counter++;
                }
            }
        }

        //If only EEG channels are used
        if (!forwardChannelsTypes.contains("MEG") && forwardChannelsTypes.contains("EEG")) {
            for (qint32 x = 0; x < std::ssize(m_pFiffInfoInput->chs); ++x) {
                if (m_pFiffInfoInput->chs[x].kind == FIFFV_EEG_CH) {
                    m_pFiffInfoForward->chs[counter].ch_name = m_pFiffInfoInput->chs[x].ch_name;
                    m_pFiffInfoForward->ch_names.push_back(m_pFiffInfoInput->chs[x].ch_name);
                    counter++;
                }
            }
        }

        //If both MEG and EEG channels are used
        if (forwardChannelsTypes.contains("MEG") && forwardChannelsTypes.contains("EEG")) {
            //qDebug()<<"RtClustMne::calcFiffInfo - MEG EEG fwd solution";
            for (qint32 x = 0; x < std::ssize(m_pFiffInfoInput->chs); ++x) {
                if (m_pFiffInfoInput->chs[x].kind == FIFFV_MEG_CH || m_pFiffInfoInput->chs[x].kind == FIFFV_EEG_CH) {
                    m_pFiffInfoForward->chs[counter].ch_name = m_pFiffInfoInput->chs[x].ch_name;
                    m_pFiffInfoForward->ch_names.push_back(m_pFiffInfoInput->chs[x].ch_name);
                    counter++;
                }
            }
        }

        //Pick only channels which are present in all data structures (covariance, evoked and forward)
        m_qListPickChannels.clear();
        for (const std::string& ch : m_pFiffInfoForward->ch_names) {
            if (mne::has_channel(m_pFiffInfoInput->ch_names, ch) && mne::has_channel(m_qListCovChNames, ch))
                m_qListPickChannels.push_back(ch);
        }
        RowVectorXi sel = m_pFiffInfoInput->pick_channels(m_qListPickChannels);

        //qDebug() << "RtClustMne::calcFiffInfo - m_qListPickChannels.size()" << m_qListPickChannels.size();
        //qDebug() << "RtClustMne::calcFiffInfo - m_qListPickChannels" << m_qListPickChannels;

        m_pFiffInfo = std::make_shared<FiffInfo>(m_pFiffInfoInput->pick_info(sel));

        m_pRTSEOutput->measurementData()->setFiffInfo(m_pFiffInfo);

        // qDebug() << "RtClustMne::calcFiffInfo - m_pFiffInfo" << m_pFiffInfo->ch_names;

        return true;
    }

    return false;
}

//=============================================================================================================

bool RtClustMne::start()
{
    QThread::start();
    return true;
}

//=============================================================================================================

bool RtClustMne::stop()
{
    requestInterruption();
    wait(500);

    m_qListCovChNames.clear();
    m_bEvokedInput = false;
    m_bRawInput = false;
    m_bPluginControlWidgetsInit = false;

    return true;
}

//=============================================================================================================

AbstractPlugin::PluginType RtClustMne::getType() const
{
    return _IAlgorithm;
}

//=============================================================================================================

QString RtClustMne::getName() const
{
    return "Source Localization";
}

//=============================================================================================================

QWidget* RtClustMne::setupWidget()
{
    RtClustMneSetupWidget* setupWidget = new RtClustMneSetupWidget(this); //widget is later distroyed by CentralWidget - so it has to be created everytime new

    return setupWidget;
}

//=============================================================================================================

void RtClustMne::updateRTFS(mne::scan::Measurement::SPtr pMeasurement)
{
    if (QSharedPointer<RealTimeFwdSolution> pRTFS = pMeasurement.dynamicCast<RealTimeFwdSolution>()) {
        //
        // Both clustered and full source spaces are accepted. Neither the inverse
        // operator nor MinimumNorm depends on the clustering, they only operate on the
        // gain matrix and the covariances. A full source space simply means
        // more sources and therefore a higher computational load.
        //
        if (!pRTFS->isClustered()) {
            Log::info("[RtClustMne::updateRTFS] Using a full (unclustered) source space. Source localization will be slower than with a clustered one.");
        }

        m_pRTSEOutput->measurementData()->setFwdSolution(pRTFS->getValue());

        // The processing thread rebuilds the operator for a new forward solution
        QMutexLocker locker(&m_qMutex);
        m_pFwd = pRTFS->getValue();
        m_pFiffInfoForward = std::make_shared<FiffInfoBase>(m_pFwd->info);
    }
}

//=============================================================================================================

void RtClustMne::updateRTMSA(mne::scan::Measurement::SPtr pMeasurement)
{
    if (m_pFwd) {
        QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<RealTimeMultiSampleArray>();

        if (pRTMSA && this->isRunning()) {
            //Fiff Information of the RTMSA
            m_qMutex.lock();
            if (!m_pFiffInfoInput) {
                m_pFiffInfoInput = pRTMSA->info();
                m_iNumAverages = 1;
                m_bRawInput = true;
            }
            m_qMutex.unlock();

            if (!m_bPluginControlWidgetsInit) {
                initPluginControlWidgets();
            }

            if (this->isRunning()) {
                // Check for artifacts
                std::map<std::string, double> mapReject;
                mapReject.emplace("eog", 150e-06);

                for (qint32 i = 0; i < pRTMSA->getMultiSampleArray().size(); ++i) {
                    bool bArtifactDetected = MNEEpochDataList::checkForArtifact(pRTMSA->getMultiSampleArray()[i],
                                                                                *m_pFiffInfoInput,
                                                                                mapReject);

                    if (!bArtifactDetected) {
                        // Please note that we do not need a copy here since this function will block until
                        // the buffer accepts new data again. Hence, the data is not deleted in the actual
                        // Measurement function after it emitted the notify signal.
                        while (!m_pCircularMatrixBuffer->push(pRTMSA->getMultiSampleArray()[i])) {
                            //Do nothing until the circular buffer is ready to accept new data again
                        }
                    } else {
                        Log::debug("RtClustMne::updateRTMSA - Reject data block");
                    }
                }
            }
        }
    }
}

//=============================================================================================================

void RtClustMne::updateRTC(mne::scan::Measurement::SPtr pMeasurement)
{
    if (m_pFwd) {
        QSharedPointer<RealTimeCov> pRTC = pMeasurement.dynamicCast<RealTimeCov>();

        if (pRTC && this->isRunning()) {
            //Fiff Information of the covariance
            if (m_qListCovChNames.size() != pRTC->getValue()->names.size()) {
                m_qListCovChNames = pRTC->getValue()->names;
            }

            if (this->isRunning()) {
                // Only the newest covariance waits for the processing thread
                QMutexLocker locker(&m_qMutex);
                m_pNoiseCov = std::make_shared<FiffCov>(*pRTC->getValue());
                m_bNewNoiseCov = true;
            }
        }
    }
}

//=============================================================================================================

void RtClustMne::updateRTE(mne::scan::Measurement::SPtr pMeasurement)
{
    if (m_pFwd) {
        if (QSharedPointer<RealTimeEvokedSet> pRTES = pMeasurement.dynamicCast<RealTimeEvokedSet>()) {
            QStringList lResponsibleTriggerTypes = pRTES->getResponsibleTriggerTypes();
            emit responsibleTriggerTypesChanged(lResponsibleTriggerTypes);

            if (!m_bPluginControlWidgetsInit) {
                initPluginControlWidgets();
            }

            if (!this->isRunning() || !lResponsibleTriggerTypes.contains(m_sAvrType)) {
                return;
            }

            std::shared_ptr<FiffEvokedSet> pFiffEvokedSet = pRTES->getValue();

            //Fiff Information of the evoked
            if (!m_pFiffInfoInput && pFiffEvokedSet->evoked.size() > 0) {
                QMutexLocker locker(&m_qMutex);

                for (int i = 0; i < std::ssize(pFiffEvokedSet->evoked); ++i) {
                    if (pFiffEvokedSet->evoked.at(i).comment == m_sAvrType) {
                        m_pFiffInfoInput = std::make_shared<FiffInfo>(pFiffEvokedSet->evoked.at(i).info);
                        break;
                    }
                }

                m_bEvokedInput = true;
            }

            if (!m_bPluginControlWidgetsInit) {
                initPluginControlWidgets();
            }

            if (this->isRunning()) {
                for (int i = 0; i < std::ssize(pFiffEvokedSet->evoked); ++i) {
                    if (pFiffEvokedSet->evoked.at(i).comment == m_sAvrType) {
                        // Store current evoked as member so we can dispatch it if the time pick by the user changed
                        m_currentEvoked = pFiffEvokedSet->evoked.at(i).pick_channels(m_qListPickChannels);

                        // Please note that we do not need a copy here since this function will block until
                        // the buffer accepts new data again. Hence, the data is not deleted in the actual
                        // Measurement function after it emitted the notify signal.
                        while (!m_pCircularEvokedBuffer->push(pFiffEvokedSet->evoked.at(i).pick_channels(m_qListPickChannels))) {
                            //Do nothing until the circular buffer is ready to accept new data again
                        }

                        //qDebug()<<"RtClustMne::updateRTE - average found type" << m_sAvrType;
                        break;
                    }
                }
            }
        }
    }
}

//=============================================================================================================

void RtClustMne::onMethodChanged(const QString& method)
{
    QMutexLocker locker(&m_qMutex);

    m_sMethod = method;
}

//=============================================================================================================

void RtClustMne::onModelCheckpointChanged(const std::filesystem::path& sPath)
{
    QMutexLocker locker(&m_qMutex);

    m_sModelCheckpoint = QString::fromStdString(mne::utf8_from_path(sPath));
}

//=============================================================================================================

void RtClustMne::onTriggerTypeChanged(const QString& triggerType)
{
    m_sAvrType = triggerType;
}

//=============================================================================================================

void RtClustMne::onTimePointValueChanged(int iTimePointMs)
{
    if (m_pFiffInfoInput && m_pCircularEvokedBuffer) {
        m_qMutex.lock();
        m_iTimePointSps = m_pFiffInfoInput->sfreq * static_cast<float>(iTimePointMs) * 0.001f;
        m_qMutex.unlock();

        if (this->isRunning()) {
            while (!m_pCircularEvokedBuffer->push(m_currentEvoked)) {
                //Do nothing until the circular buffer is ready to accept new data again
            }
        }
    }
}

//=============================================================================================================

void RtClustMne::run()
{
    // Wait for fiff info to arrive
    while (!calcFiffInfo()) {
        if (isInterruptionRequested()) {
            return;
        }
        msleep(200);
    }

    m_qMutex.lock();
    const bool bEvokedInput = m_bEvokedInput;
    const bool bRawInput = m_bRawInput;
    const FiffInfo info = *m_pFiffInfo;
    const float tstep = 1.0f / m_pFiffInfoInput->sfreq;
    // Raw blocks hold every input channel, the operator expects those of m_pFiffInfo
    std::vector<int> rawPicks;
    for (const std::string& name : info.ch_names) {
        rawPicks.push_back(mne::channel_index(m_pFiffInfoInput->ch_names, name));
    }
    m_qMutex.unlock();

    // A single raw block is localized at SNR 1, an average at SNR 3 (mne.minimum_norm.apply_inverse)
    const float lambda2 = bEvokedInput ? 1.0f / 9.0f : 1.0f;
    std::optional<RtInvOp> rtInvOp;
    std::shared_ptr<MNEForwardSolution> fwd;
    QString sMethod;
    VectorXi vertices;
    int nVerticesLh = 0;
    qint32 blockCount = 0;
    MatrixXd matData;
    FiffEvoked evoked;

    const auto publish = [&](InvSourceEstimate sourceEstimate, int iTimePointSps) {
        if (sourceEstimate.empty() || sourceEstimate.data.size() == 0) {
            return;
        }
        if (iTimePointSps >= 0 && iTimePointSps < sourceEstimate.data.cols()) {
            sourceEstimate = sourceEstimate.reduce(iTimePointSps, 1);
        }
        m_pRTSEOutput->measurementData()->setValue(sourceEstimate);
    };
    const auto estimate = [&](MatrixXd sources, float tmin) {
        InvSourceEstimate stc(std::move(sources), vertices, tmin, tstep);
        stc.nVerticesLh = nVerticesLh;
        return stc;
    };

    // Start processing data
    while (!isInterruptionRequested()) {
        m_qMutex.lock();
        const int iTimePointSps = m_iTimePointSps;
        const int iDownSample = std::max(1, m_iDownSample);
        const QString sModelCheckpoint = m_sModelCheckpoint;
        const bool rebuild = m_pFwd && (m_pFwd != fwd || m_sMethod != sMethod);
        if (rebuild) {
            fwd = m_pFwd;
            sMethod = m_sMethod;
        }
        std::shared_ptr<FiffCov> pNoiseCov;
        if (m_pNoiseCov && (rebuild || m_bNewNoiseCov)) {
            pNoiseCov = m_pNoiseCov;
            m_bNewNoiseCov = false;
        }
        m_qMutex.unlock();
        const bool bCmne = sMethod == QLatin1String("CMNE");

        if (rebuild) {
            // CMNE takes the covariances and the gain of a dSPM operator
            const InvEstimateMethod method = sMethod == QLatin1String("MNE") ? InvEstimateMethod::MNE
                : sMethod == QLatin1String("sLORETA")                        ? InvEstimateMethod::sLORETA
                                                                             : InvEstimateMethod::dSPM;
            rtInvOp.emplace(info, fwd->pick_types(true, false), RtInvOp::Settings{.method = method, .lambda2 = lambda2, .pick_normal = !bEvokedInput});
            vertices.resize(fwd->src[0].vertno.size() + fwd->src[1].vertno.size());
            vertices << fwd->src[0].vertno, fwd->src[1].vertno;
            nVerticesLh = static_cast<int>(fwd->src[0].vertno.size());
        }
        if (pNoiseCov && !rtInvOp->update(*pNoiseCov)) {
            Log::warning("[RtClustMne::run] The noise covariance gives no inverse operator.");
        }

        //Process data from raw data input
        if (bRawInput && m_pCircularMatrixBuffer->pop(matData) && ++blockCount % iDownSample == 0 && rtInvOp && rtInvOp->ready()) {
            publish(estimate(rtInvOp->apply(matData(rawPicks, placeholders::all)), 0.0f), iTimePointSps);
        }

        //Process data from averaging input
        if (bEvokedInput && m_pCircularEvokedBuffer->pop(evoked) && ++blockCount % iDownSample == 0 && rtInvOp && rtInvOp->ready()) {
            if (bCmne) {
                publish(computeCmneInverse(evoked, *rtInvOp, lambda2, sModelCheckpoint), iTimePointSps);
            } else {
                // Averages that arrived before the channel selection still hold every channel
                std::vector<int> rows;
                for (const std::string& name : info.ch_names) {
                    rows.push_back(mne::channel_index(evoked.info.ch_names, name));
                }
                if (!std::ranges::contains(rows, -1)) {
                    publish(estimate(rtInvOp->apply(evoked.data(rows, placeholders::all), evoked.nave), evoked.times.size() > 0 ? evoked.times[0] : 0.0f), iTimePointSps);
                }
            }
        }
    }
}

//=============================================================================================================

InvSourceEstimate RtClustMne::computeCmneInverse(const FiffEvoked& evoked,
                                                 const RtInvOp& inverse,
                                                 float fLambda2,
                                                 const QString& sModelCheckpoint)
{
    InvSourceEstimate stcEmpty;

    if (sModelCheckpoint.isEmpty()) {
        Log::warning("[RtClustMne::computeCmneInverse] CMNE selected but no model checkpoint set.");
        return stcEmpty;
    }

    // The gain, the covariances and the data rows all belong to the operator's channels
    const FiffCov& noiseCov = inverse.noise_covariance();
    std::vector<int> rows;
    for (const std::string& name : noiseCov.names) {
        rows.push_back(mne::channel_index(evoked.info.ch_names, name));
    }
    if (std::ranges::contains(rows, -1)) {
        Log::warning("[RtClustMne::computeCmneInverse] The evoked lacks channels of the inverse operator.");
        return stcEmpty;
    }

    InvCMNESettings cmneSettings;
    cmneSettings.onnxModelPath = sModelCheckpoint.toStdString();
    cmneSettings.lambda2 = static_cast<double>(fLambda2);
    cmneSettings.numSources = static_cast<int>(inverse.gain().cols());

    InvCMNEResult res = InvCMNE::compute(evoked.data(rows, placeholders::all),
                                         inverse.gain(),
                                         noiseCov.data,
                                         MatrixXd(inverse.source_covariance().asDiagonal()),
                                         cmneSettings);
    return res.stcCmne;
}

//=============================================================================================================

QString RtClustMne::getBuildInfo()
{
    return QString(RTCLUSTMNEPLUGIN::buildDateTime()) + QString(" - ") + QString(RTCLUSTMNEPLUGIN::buildHash());
}

//=============================================================================================================

QVariantMap RtClustMne::getAttributes() const
{
    QVariantMap attrs;
    attrs[QStringLiteral("atlasDir")] = m_sAtlasDir;
    attrs[QStringLiteral("surfaceDir")] = m_sSurfaceDir;
    attrs[QStringLiteral("mriHeadTrans")] = m_fMriHeadTrans.fileName();
    attrs[QStringLiteral("method")] = m_sMethod;
    attrs[QStringLiteral("avrType")] = m_sAvrType;
    attrs[QStringLiteral("downSample")] = m_iDownSample;
    attrs[QStringLiteral("numAverages")] = m_iNumAverages;
    attrs[QStringLiteral("modelCheckpoint")] = m_sModelCheckpoint;
    return attrs;
}

//=============================================================================================================

void RtClustMne::setAttributes(const QVariantMap& attributes)
{
    if (attributes.contains(QStringLiteral("atlasDir")))
        m_sAtlasDir = attributes[QStringLiteral("atlasDir")].toString();
    if (attributes.contains(QStringLiteral("surfaceDir")))
        m_sSurfaceDir = attributes[QStringLiteral("surfaceDir")].toString();
    if (attributes.contains(QStringLiteral("mriHeadTrans")))
        m_fMriHeadTrans.setFileName(attributes[QStringLiteral("mriHeadTrans")].toString());
    if (attributes.contains(QStringLiteral("method")))
        m_sMethod = attributes[QStringLiteral("method")].toString();
    if (attributes.contains(QStringLiteral("avrType")))
        m_sAvrType = attributes[QStringLiteral("avrType")].toString();
    if (attributes.contains(QStringLiteral("downSample")))
        m_iDownSample = attributes[QStringLiteral("downSample")].toInt();
    if (attributes.contains(QStringLiteral("numAverages")))
        m_iNumAverages = attributes[QStringLiteral("numAverages")].toInt();
    if (attributes.contains(QStringLiteral("modelCheckpoint")))
        m_sModelCheckpoint = attributes[QStringLiteral("modelCheckpoint")].toString();
}
