//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     rtfwd.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           johaenns <j.vorw01@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>
 * @since    0.1.1
 * @date     May, 2020
 * @brief    Definition of the RtFwd class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/log.h>
#include "rtfwd.h"

#include "FormFiles/rtfwdsetupwidget.h"
#include <mne/viz/backends/qt/viewers/fwdsettingsview.h>
#include <mne/fwd/compute_fwd/compute_fwd_settings.h>
#include <mne/fwd/compute_fwd/compute_fwd.h>
#include <mne/inv/hpi/inv_hpi_fit.h>
#include <mne/fs/fs_annotationset.h>
#include <mne/mne/mne_forward_solution.h>
#include <mne/base/string_utils.h>
#include <scMeas/realtimehpiresult.h>
#include <scMeas/realtimefwdsolution.h>
#include <scMeas/realtimemultisamplearray.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
#include <QFile>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTFWDPLUGIN;
using namespace mne::scan;
using namespace mne;
using namespace mne::viz::qt;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtFwd::RtFwd()
: m_pFwdSettings(std::make_shared<ComputeFwdSettings>())
, m_bBusy(false)
, m_bDoRecomputation(false)
, m_bDoClustering(true)
, m_bNClusterChanged(true)
, m_bDoFwdComputation(false)
{
    // set init values
    m_pFwdSettings->solname = (QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/your-solution-fwd.fif").toStdString();
    m_pFwdSettings->mriname = (QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/all-trans.fif").toStdString();
    m_pFwdSettings->bemname = (QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/subjects/sample/bem/sample-5120-5120-5120-bem.fif").toStdString();
    m_pFwdSettings->srcname = (QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/subjects/sample/bem/sample-oct-6-src.fif").toStdString();
    m_pFwdSettings->measname = (QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/MEG/sample/sample_audvis_raw.fif").toStdString();
    m_pFwdSettings->transname.clear();
    m_pFwdSettings->eeg_model_name = "Default";
    m_pFwdSettings->include_meg = true;
    m_pFwdSettings->include_eeg = true;
    m_pFwdSettings->accurate = true;
    m_pFwdSettings->mindist = 5.0f / 1000.0f;
    m_pFwdSettings->ncluster = 200;

    m_sAtlasDir = QCoreApplication::applicationDirPath() + "/../resources/data/MNE-sample-data/subjects/sample/label";
}

//=============================================================================================================

RtFwd::~RtFwd()
{
    if (this->isRunning()) {
        stop();
    }
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> RtFwd::clone() const
{
    auto pRtFwdClone = QSharedPointer<RtFwd>::create();
    return pRtFwdClone;
}

//=============================================================================================================

void RtFwd::init()
{
    // Inits
    m_pAnnotationSet = std::make_shared<FsAnnotationSet>((m_sAtlasDir + "/lh.aparc.a2009s.annot").toStdU16String(), (m_sAtlasDir + "/rh.aparc.a2009s.annot").toStdU16String());

    // Input
    m_pHpiInput = PluginInputData<RealTimeHpiResult>::create(this, "rtFwd RTHR In", "rtFwd real time HPI result input data");
    connect(m_pHpiInput.data(), &PluginInputConnector::notify,
            this, &RtFwd::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pHpiInput);

    m_pRTMSAInput = PluginInputData<RealTimeMultiSampleArray>::create(this, "rtFwd RTMSA In", "rtFwd real-time multi sample array input data");
    connect(m_pRTMSAInput.data(), &PluginInputConnector::notify,
            this, &RtFwd::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTMSAInput);

    // Output
    m_pRTFSOutput = PluginOutputData<RealTimeFwdSolution>::create(this, "rtFwdOut", "rtFwd real-time forward solution output data");
    m_pRTFSOutput->measurementData()->setName(this->getName()); //Provide name to auto store widget settings
    m_outputConnectors.append(m_pRTFSOutput);
}

//=============================================================================================================

void RtFwd::unload()
{
}

//=============================================================================================================

bool RtFwd::start()
{
    // Maybe we can move all of this to the run() method?
    const std::pair<const std::string*, const char*> inputs[] = {
        {&m_pFwdSettings->bemname, "The bem model cannot be opend. Chosse another file."},
        {&m_pFwdSettings->srcname, "The source space cannot be opend. Chosse another file."},
        {&m_pFwdSettings->mriname, "The mri - head transformation cannot be opend. Chosse another file."},
        {&m_pFwdSettings->measname, "The meaurement file cannot be opend. Chosse another file."},
    };
    for (const auto& [path, message] : inputs) {
        if (!FiffStream::open(mne::path_from_utf8(*path))) {
            QMessageBox msgBox;
            msgBox.setText(message);
            msgBox.setInformativeText(QString::fromStdString(*path));
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setWindowFlags(Qt::WindowStaysOnTopHint);
            msgBox.exec();
            return false;
        }
    }

    //Start thread
    QThread::start();

    return true;
}

//=============================================================================================================

bool RtFwd::stop()
{
    requestInterruption();
    wait(500);

    m_bPluginControlWidgetsInit = false;

    return true;
}

//=============================================================================================================

AbstractPlugin::PluginType RtFwd::getType() const
{
    return _IAlgorithm;
}

//=============================================================================================================

QString RtFwd::getName() const
{
    return "Forward Solution";
}

//=============================================================================================================

QWidget* RtFwd::setupWidget()
{
    RtFwdSetupWidget* setupWidget = new RtFwdSetupWidget(this);
    return setupWidget;
}

//=============================================================================================================

void RtFwd::update(mne::scan::Measurement::SPtr pMeasurement)
{
    if (QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<RealTimeMultiSampleArray>()) {
        //Fiff information
        m_mutex.lock();
        if (!m_pFiffInfo) {
            m_pFiffInfo = pRTMSA->info();
        }
        m_mutex.unlock();

        if (!m_bPluginControlWidgetsInit) {
            initPluginControlWidgets();
        }
    } else if (QSharedPointer<RealTimeHpiResult> pRTHPI = pMeasurement.dynamicCast<RealTimeHpiResult>()) {
        //Fiff information
        m_mutex.lock();
        if (!m_pFiffInfo) {
            m_pFiffInfo = pRTHPI->getFiffInfo();
        }

        if (!m_bBusy) {
            m_pHpiFitResult = pRTHPI->getValue();
        }
        m_mutex.unlock();

        if (!m_bPluginControlWidgetsInit) {
            initPluginControlWidgets();
        }
    }
}

//=============================================================================================================

void RtFwd::initPluginControlWidgets()
{
    bool bFiffInfo = false;
    m_mutex.lock();
    if (m_pFiffInfo) {
        bFiffInfo = true;
    }
    m_mutex.unlock();

    if (bFiffInfo) {
        std::vector<std::unique_ptr<QWidget>> controlWidgets;

        FwdSettingsView* pFwdSettingsView = makeControlWidget<FwdSettingsView>(controlWidgets, QString("MNESCAN/%1/").arg(this->getName()));
        connect(this, &RtFwd::guiModeChanged,
                pFwdSettingsView, &FwdSettingsView::setGuiMode);
        pFwdSettingsView->setObjectName("widget_");

        // connect incoming signals
        connect(pFwdSettingsView, &FwdSettingsView::recompStatusChanged,
                this, &RtFwd::onRecompStatusChanged);
        connect(pFwdSettingsView, &FwdSettingsView::clusteringStatusChanged,
                this, &RtFwd::onClusteringStatusChanged);
        connect(pFwdSettingsView, &FwdSettingsView::atlasDirChanged,
                this, &RtFwd::onAtlasDirChanged);
        connect(pFwdSettingsView, &FwdSettingsView::clusterNumberChanged,
                this, &RtFwd::onClusterNumberChanged);
        connect(pFwdSettingsView, &FwdSettingsView::doForwardComputation,
                this, &RtFwd::onDoForwardComputation);

        // connect outgoing signals
        connect(this, &RtFwd::statusInformationChanged,
                pFwdSettingsView, &FwdSettingsView::setRecomputationStatus, Qt::BlockingQueuedConnection);
        connect(this, &RtFwd::fwdSolutionAvailable,
                pFwdSettingsView, &FwdSettingsView::setSolutionInformation, Qt::BlockingQueuedConnection);
        connect(this, &RtFwd::clusteringAvailable,
                pFwdSettingsView, &FwdSettingsView::setClusteredInformation, Qt::BlockingQueuedConnection);


        offerControlWidgets(std::move(controlWidgets));

        m_bPluginControlWidgetsInit = true;
    }
}

//=============================================================================================================

void RtFwd::onDoForwardComputation()
{
    m_mutex.lock();
    m_bDoFwdComputation = true;
    m_mutex.unlock();
}

//=============================================================================================================

void RtFwd::onRecompStatusChanged(bool bDoRecomputation)
{
    m_mutex.lock();
    if (!m_pHpiInput) {
        QMessageBox msgBox;
        msgBox.setText("Please connect the Hpi plugin.");
        msgBox.exec();
        return;
    }
    m_bDoRecomputation = bDoRecomputation;
    m_mutex.unlock();
}

//=============================================================================================================

void RtFwd::onClusteringStatusChanged(bool bDoClustering)
{
    if (m_pAnnotationSet->empty()) {
        QMessageBox msgBox;
        msgBox.setText("Please load an annotation set befor clustering.");
        msgBox.exec();
        return;
    }
    m_mutex.lock();
    m_bDoClustering = bDoClustering;
    m_mutex.unlock();
}

//=============================================================================================================

void RtFwd::onAtlasDirChanged(const std::filesystem::path& sDirPath, const std::shared_ptr<FsAnnotationSet>& pAnnotationSet)
{
    m_mutex.lock();
    m_sAtlasDir = QString::fromStdString(mne::utf8_from_path(sDirPath));
    m_pAnnotationSet = pAnnotationSet;
    m_mutex.unlock();
}

//=============================================================================================================

void RtFwd::onClusterNumberChanged(int iNClusterNumber)
{
    m_mutex.lock();
    m_pFwdSettings->ncluster = iNClusterNumber;
    m_bNClusterChanged = true;
    m_mutex.unlock();
}

//=============================================================================================================

void RtFwd::run()
{
    // Wait for fiff the info to arrive
    while (!isInterruptionRequested()) {
        m_mutex.lock();
        if (m_pFiffInfo) {
            m_mutex.unlock();
            break;
        }
        m_mutex.unlock();
        msleep(200);
    }
    if (isInterruptionRequested()) {
        return;
    }

    m_mutex.lock();
    m_pFwdSettings->pFiffInfo = m_pFiffInfo;
    m_pRTFSOutput->measurementData()->setFiffInfo(m_pFiffInfo);
    FiffCoordTrans transMegHead = m_transDevHead;
    m_mutex.unlock();

    // initialize fwd solution
    emit statusInformationChanged(0); // initializing
    std::shared_ptr<ComputeFwd> pFwdComputer = std::make_shared<ComputeFwd>(m_pFwdSettings);

    std::shared_ptr<MNEForwardSolution> pFwdSolution;
    std::shared_ptr<MNEForwardSolution> pClusteredFwd;

    emit statusInformationChanged(4); // not computed

    // do recomputation if requested, not busy and transformation is different
    bool bIsLargeHeadMovement = false; // indicate if movement was large
    bool bIsDifferent = false;         // indicate if incoming transformation matrix is different
    bool bDoRecomputation = false;     // indicate if we want to recompute
    bool bDoClustering = false;        // indicate if we want to cluster
    bool bFwdReady = false;            // only cluster if fwd is ready
    bool bNClusterChanged = false;     // Perform new clustering when cluster size changed
    bool bHpiConnectected = false;     // only update/recompute if hpi is connected
    bool bDoFwdComputation = false;    // compute forward if requested
    bool bIsInit = false;              // only recompute if initial fwd solulion is calculated

    while (!isInterruptionRequested()) {
        m_mutex.lock();
        bDoFwdComputation = m_bDoFwdComputation;
        m_mutex.unlock();

        if (bDoFwdComputation) {
            emit statusInformationChanged(1); // computing
            m_mutex.lock();
            m_bBusy = true;
            m_mutex.unlock();

            // compute and store
            pFwdSolution.reset(pFwdComputer->calculateFwd().release());

            if (auto written = pFwdSolution->write(mne::path_from_utf8(m_pFwdSettings->solname)); !written) {
                Log::warning("{}", written.error().what());
            }

            // emit results to control widget
            emit fwdSolutionAvailable(pFwdSolution->source_ori,
                                      pFwdSolution->coord_frame,
                                      pFwdSolution->nsource,
                                      pFwdSolution->nchan,
                                      pFwdSolution->src.size());

            m_mutex.lock();
            if (!m_bDoClustering) {
                m_pRTFSOutput->measurementData()->setValue(pFwdSolution);
                emit statusInformationChanged(5); //finished
            }
            bFwdReady = true;            // provide fwd for clustering if wanted
            m_bDoFwdComputation = false; // don't call this again if not requested
            bIsInit = true;              // init computation finished -> recomputation possible
            m_mutex.unlock();
        }

        // check if hpi is connected
        m_mutex.lock();
        if (m_pHpiFitResult) {
            bHpiConnectected = true;
        }
        m_mutex.unlock();

        if (bHpiConnectected && bIsInit) {
            // only recompute if hpi is connected
            m_mutex.lock();
            bIsLargeHeadMovement = m_pHpiFitResult->bIsLargeHeadMovement;
            bIsDifferent = !(transMegHead == m_pHpiFitResult->devHeadTrans);
            bDoRecomputation = m_bDoRecomputation;
            m_mutex.unlock();

            // do recomputation if requested, a large head movement occured and devHeadTrans is different
            if (bIsLargeHeadMovement && bIsDifferent && bDoRecomputation) {
                emit statusInformationChanged(2); // recomputing
                m_mutex.lock();
                m_bBusy = true;
                transMegHead = m_pHpiFitResult->devHeadTrans;
                m_mutex.unlock();

                pFwdComputer->updateHeadPos(transMegHead, *pFwdSolution);

                m_mutex.lock();
                m_bBusy = false;
                m_mutex.unlock();
                bFwdReady = true;

                if (!bDoClustering) {
                    m_pRTFSOutput->measurementData()->setValue(pFwdSolution);
                    //bFwdReady = false; // doesn't seem to be necessary? bDoClustering = false anyway
                    emit statusInformationChanged(5); //finished
                }
            }
        }

        // do clustering if requested and fwd is ready
        m_mutex.lock();
        bDoClustering = m_bDoClustering;
        bNClusterChanged = m_bNClusterChanged;
        m_mutex.unlock();

        if (bDoClustering && bFwdReady && bNClusterChanged) {
            emit statusInformationChanged(3); // clustering
            pClusteredFwd = std::make_shared<MNEForwardSolution>(pFwdSolution->cluster_forward_solution(*m_pAnnotationSet, m_pFwdSettings->ncluster));
            emit clusteringAvailable(pClusteredFwd->nsource);

            m_pRTFSOutput->measurementData()->setValue(pClusteredFwd);

            //bFwdReady = false; // fwd remains ready, allows for reclustering.
            m_mutex.lock();
            m_bNClusterChanged = false;
            m_mutex.unlock();

            emit statusInformationChanged(6); //finished
        }
    }
}

//=============================================================================================================

QString RtFwd::getBuildInfo()
{
    return QString(RTFWDPLUGIN::buildDateTime()) + QString(" - ") + QString(RTFWDPLUGIN::buildHash());
}

//=============================================================================================================

QVariantMap RtFwd::getAttributes() const
{
    QVariantMap attrs;
    attrs[QStringLiteral("solname")] = QString::fromStdString(m_pFwdSettings->solname);
    attrs[QStringLiteral("mriname")] = QString::fromStdString(m_pFwdSettings->mriname);
    attrs[QStringLiteral("bemname")] = QString::fromStdString(m_pFwdSettings->bemname);
    attrs[QStringLiteral("srcname")] = QString::fromStdString(m_pFwdSettings->srcname);
    attrs[QStringLiteral("measname")] = QString::fromStdString(m_pFwdSettings->measname);
    attrs[QStringLiteral("transname")] = QString::fromStdString(m_pFwdSettings->transname);
    attrs[QStringLiteral("eeg_model_name")] = QString::fromStdString(m_pFwdSettings->eeg_model_name);
    attrs[QStringLiteral("include_meg")] = m_pFwdSettings->include_meg;
    attrs[QStringLiteral("include_eeg")] = m_pFwdSettings->include_eeg;
    attrs[QStringLiteral("accurate")] = m_pFwdSettings->accurate;
    attrs[QStringLiteral("mindist")] = static_cast<double>(m_pFwdSettings->mindist);
    attrs[QStringLiteral("ncluster")] = m_pFwdSettings->ncluster;
    attrs[QStringLiteral("atlasDir")] = m_sAtlasDir;
    attrs[QStringLiteral("doClustering")] = m_bDoClustering;
    attrs[QStringLiteral("doRecomputation")] = m_bDoRecomputation;
    return attrs;
}

//=============================================================================================================

void RtFwd::setAttributes(const QVariantMap& attributes)
{
    if (attributes.contains(QStringLiteral("solname")))
        m_pFwdSettings->solname = attributes[QStringLiteral("solname")].toString().toStdString();
    if (attributes.contains(QStringLiteral("mriname")))
        m_pFwdSettings->mriname = attributes[QStringLiteral("mriname")].toString().toStdString();
    if (attributes.contains(QStringLiteral("bemname")))
        m_pFwdSettings->bemname = attributes[QStringLiteral("bemname")].toString().toStdString();
    if (attributes.contains(QStringLiteral("srcname")))
        m_pFwdSettings->srcname = attributes[QStringLiteral("srcname")].toString().toStdString();
    if (attributes.contains(QStringLiteral("measname")))
        m_pFwdSettings->measname = attributes[QStringLiteral("measname")].toString().toStdString();
    if (attributes.contains(QStringLiteral("transname")))
        m_pFwdSettings->transname = attributes[QStringLiteral("transname")].toString().toStdString();
    if (attributes.contains(QStringLiteral("eeg_model_name")))
        m_pFwdSettings->eeg_model_name = attributes[QStringLiteral("eeg_model_name")].toString().toStdString();
    if (attributes.contains(QStringLiteral("include_meg")))
        m_pFwdSettings->include_meg = attributes[QStringLiteral("include_meg")].toBool();
    if (attributes.contains(QStringLiteral("include_eeg")))
        m_pFwdSettings->include_eeg = attributes[QStringLiteral("include_eeg")].toBool();
    if (attributes.contains(QStringLiteral("accurate")))
        m_pFwdSettings->accurate = attributes[QStringLiteral("accurate")].toBool();
    if (attributes.contains(QStringLiteral("mindist")))
        m_pFwdSettings->mindist = static_cast<float>(attributes[QStringLiteral("mindist")].toDouble());
    if (attributes.contains(QStringLiteral("ncluster")))
        m_pFwdSettings->ncluster = attributes[QStringLiteral("ncluster")].toInt();
    if (attributes.contains(QStringLiteral("atlasDir")))
        m_sAtlasDir = attributes[QStringLiteral("atlasDir")].toString();
    if (attributes.contains(QStringLiteral("doClustering")))
        m_bDoClustering = attributes[QStringLiteral("doClustering")].toBool();
    if (attributes.contains(QStringLiteral("doRecomputation")))
        m_bDoRecomputation = attributes[QStringLiteral("doRecomputation")].toBool();
}
