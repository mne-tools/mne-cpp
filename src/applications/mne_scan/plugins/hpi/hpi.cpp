//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     hpi.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2020
 * @brief    Definition of the Hpi class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/base/log.h>
#include "hpi.h"

#include "FormFiles/hpisetupwidget.h"
#include <mne/base/string_utils.h>
#include <mne/disp/viewers/hpisettingsview.h>
#include <scMeas/realtimemultisamplearray.h>
#include <scMeas/realtimehpiresult.h>
#include <mne/dsp/rt/rt_hpi.h>
#include <mne/fiff/fiff_info.h>
#include <mne/fiff/fiff_digitizer_data.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDir>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>
#include <cmath>
#include <filesystem>
#include <numeric>
#include <optional>
#include <utility>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace HPIPLUGIN;
using namespace mne::scan;
using namespace mne::disp;
using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE LOCAL CONSTANTS
//=============================================================================================================

constexpr const int defaultFittingWindowSize(300);

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Hpi::Hpi()
: m_iNumberBadChannels(0)
, m_iFittingWindowSize(defaultFittingWindowSize)
, m_dAllowedMeanErrorDist(10)
, m_dAllowedMovement(3)
, m_dAllowedRotation(5)
, m_bDoFreqOrder(false)
, m_bDoSingleHpi(false)
, m_bDoContinousHpi(false)
, m_bUseSSP(false)
, m_bUseComp(false)
, m_pCircularBuffer(std::make_shared<mne::CircularBuffer<Eigen::MatrixXd>>(40))
{
    connect(this, &Hpi::devHeadTransAvailable,
            this, &Hpi::onDevHeadTransAvailable, Qt::BlockingQueuedConnection);
}

//=============================================================================================================

Hpi::~Hpi()
{
    if (isRunning()) {
        resetState();
    }
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> Hpi::clone() const
{
    QSharedPointer<Hpi> pHpiClone(new Hpi);
    return pHpiClone;
}

//=============================================================================================================

void Hpi::init()
{
    // Input
    m_pHpiInput = PluginInputData<RealTimeMultiSampleArray>::create(this, "HpiIn", "Hpi input data");
    connect(m_pHpiInput.data(), &PluginInputConnector::notify,
            this, &Hpi::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pHpiInput);

    // Output
    m_pHpiOutput = PluginOutputData<RealTimeHpiResult>::create(this, "HpiOut", "Hpi output data");
    m_pHpiOutput->measurementData()->setName(this->getName()); //Provide name to auto store widget settings
    m_outputConnectors.append(m_pHpiOutput);
}

//=============================================================================================================

void Hpi::unload()
{
}

//=============================================================================================================

bool Hpi::start()
{
    QThread::start();

    return true;
}

//=============================================================================================================

bool Hpi::stop()
{
    requestInterruption();
    resetState();
    return true;
}

//=============================================================================================================

void Hpi::resetState()
{
    m_bPluginControlWidgetsInit = false;
    m_pCircularBuffer->clear();

    m_pFiffInfo.reset();
    m_pFiffDigitizerData.reset();
}

//=============================================================================================================

AbstractPlugin::PluginType Hpi::getType() const
{
    return _IAlgorithm;
}

//=============================================================================================================

QString Hpi::getName() const
{
    return "HPI Fitting";
}

//=============================================================================================================

QWidget* Hpi::setupWidget()
{
    HpiSetupWidget* setupWidget = new HpiSetupWidget(this); //widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}

//=============================================================================================================

void Hpi::update(mne::scan::Measurement::SPtr pMeasurement)
{
    if (QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<RealTimeMultiSampleArray>()) {
        //Check if the fiff info was inititalized

        manageInitialization(pRTMSA);

        // Check if data is present
        if (pRTMSA->getMultiSampleArray().size() > 0) {
            //If bad channels changed, recalcluate projectors
            updateProjections();

            m_mutex.lock();
            bool bDoSingleHpi = m_bDoSingleHpi;
            bool bDoFreqOrder = m_bDoFreqOrder;
            m_mutex.unlock();

            if (bDoFreqOrder || bDoSingleHpi) {
                while (!m_pCircularBuffer->push(pRTMSA->getMultiSampleArray()[0])) {
                    //Do nothing until the circular buffer is ready to accept new data again
                }
            }

            if (m_bDoContinousHpi && (m_vCoilFreqs.size() >= 3)) {
                for (qsizetype i = 0; i < pRTMSA->getMultiSampleArray().size(); ++i) {
                    // Please note that we do not need a copy here since this function will block until
                    // the buffer accepts new data again. Hence, the data is not deleted in the actual
                    // Measurement function after it emitted the notify signal.
                    while (!m_pCircularBuffer->push(pRTMSA->getMultiSampleArray()[i])) {
                        //Do nothing until the circular buffer is ready to accept new data again
                    }
                }
            }
        }
    }
}

//=============================================================================================================

void Hpi::manageInitialization(QSharedPointer<mne::scan::RealTimeMultiSampleArray> pRTMSA)
{
    if (!m_pFiffInfo) {
        initFiffInfo(pRTMSA->info());
    }
    if (!m_bPluginControlWidgetsInit) {
        initPluginControlWidgets();
    }
    if (!m_pFiffDigitizerData && m_pFiffInfo) {
        initFiffDigitizers(pRTMSA->digitizerData());
    }
}

//=============================================================================================================

void Hpi::initFiffInfo(std::shared_ptr<mne::FiffInfo> info)
{
    m_mutex.lock();
    m_pFiffInfo = std::move(info);
    m_pHpiOutput->measurementData()->setFiffInfo(m_pFiffInfo);

    // Take the HPI coil excitation frequencies from the file when it provides
    // them, so the user does not have to type them in by hand. Frequencies the
    // user enters in the GUI still override these.
    if (m_vCoilFreqs.empty() && m_pFiffInfo && !m_pFiffInfo->hpi_coil_freqs.empty()) {
        for (float fFreq : m_pFiffInfo->hpi_coil_freqs) {
            m_vCoilFreqs.push_back(static_cast<int>(std::lround(fFreq)));
        }

        StringList frequencies;
        for (const int frequency : m_vCoilFreqs) {
            frequencies.push_back(std::to_string(frequency));
        }
        Log::info("[Hpi::initFiffInfo] Read {} HPI coil frequencies from the file: {}", m_vCoilFreqs.size(), join(frequencies, ", "));
    }
    m_mutex.unlock();
    updateProjections();
}

//=============================================================================================================

void Hpi::initFiffDigitizers(const std::shared_ptr<mne::FiffDigitizerData>& fiffDig)
{
    if (fiffDig) {
        m_mutex.lock();
        m_pFiffDigitizerData = fiffDig;
        m_pHpiOutput->measurementData()->setDigitizerData(m_pFiffDigitizerData);
        m_pFiffInfo->dig = m_pFiffDigitizerData->points; //temp solution. refactor fit function so this isn't necessary.
        m_mutex.unlock();

        updateDigitizerInfo();
    }
}

//=============================================================================================================

void Hpi::updateDigitizerInfo()
{
    emit newDigitizerList(m_pFiffDigitizerData->points);
}

//=============================================================================================================

void Hpi::initPluginControlWidgets()
{
    bool bFiffInfo = false;
    m_mutex.lock();
    if (m_pFiffInfo) {
        bFiffInfo = true;
    }
    m_mutex.unlock();

    if (bFiffInfo) {
        QList<QWidget*> plControlWidgets;

        // Projects Settings
        HpiSettingsView* pHpiSettingsView = new HpiSettingsView(QString("MNESCAN/%1/").arg(this->getName()));

        connect(this, &Hpi::guiModeChanged,
                pHpiSettingsView, &HpiSettingsView::setGuiMode);
        pHpiSettingsView->setObjectName("widget_");

        connect(pHpiSettingsView, &HpiSettingsView::digitizersChanged,
                this, &Hpi::onDigitizersChanged);
        connect(pHpiSettingsView, &HpiSettingsView::doFreqOrder,
                this, &Hpi::onDoFreqOrder);
        connect(pHpiSettingsView, &HpiSettingsView::doSingleHpiFit,
                this, &Hpi::onDoSingleHpiFit);
        connect(pHpiSettingsView, &HpiSettingsView::coilFrequenciesChanged,
                this, &Hpi::onCoilFrequenciesChanged);
        connect(pHpiSettingsView, &HpiSettingsView::sspStatusChanged,
                this, &Hpi::onSspStatusChanged);
        connect(pHpiSettingsView, &HpiSettingsView::compStatusChanged,
                this, &Hpi::onCompStatusChanged);
        connect(pHpiSettingsView, &HpiSettingsView::contHpiStatusChanged,
                this, &Hpi::onContHpiStatusChanged);
        connect(pHpiSettingsView, &HpiSettingsView::allowedMeanErrorDistChanged,
                this, &Hpi::onAllowedMeanErrorDistChanged);
        connect(pHpiSettingsView, &HpiSettingsView::allowedMovementChanged,
                this, &Hpi::onAllowedMovementChanged);
        connect(pHpiSettingsView, &HpiSettingsView::allowedRotationChanged,
                this, &Hpi::onAllowedRotationChanged);
        connect(pHpiSettingsView, &HpiSettingsView::fittingWindowSizeChanged,
                this, &Hpi::setFittingWindowSize);
        connect(this, &Hpi::errorsChanged,
                pHpiSettingsView, &HpiSettingsView::setErrorLabels, Qt::BlockingQueuedConnection);
        connect(this, &Hpi::gofChanged,
                pHpiSettingsView, &HpiSettingsView::setGoFLabels, Qt::BlockingQueuedConnection);
        connect(this, &Hpi::movementResultsChanged,
                pHpiSettingsView, &HpiSettingsView::setMovementResults, Qt::BlockingQueuedConnection);
        connect(this, &Hpi::newDigitizerList,
                pHpiSettingsView, &HpiSettingsView::newDigitizerList);

        onSspStatusChanged(pHpiSettingsView->getSspStatusChanged());
        onCompStatusChanged(pHpiSettingsView->getCompStatusChanged());
        onAllowedMeanErrorDistChanged(pHpiSettingsView->getAllowedMeanErrorDistChanged());
        onAllowedMovementChanged(pHpiSettingsView->getAllowedMovementChanged());
        onAllowedRotationChanged(pHpiSettingsView->getAllowedRotationChanged());
        setFittingWindowSize(pHpiSettingsView->getFittingWindowSize());
        onContHpiStatusChanged(pHpiSettingsView->continuousHPIChecked());

        // The digitizers may already have been loaded before this view existed,
        // in which case the newDigitizerList signal was emitted with nobody
        // connected to it. Push the current list so they show up right away.
        m_mutex.lock();
        const bool bHaveDigitizers = !!m_pFiffDigitizerData;
        const mne::FiffDigPointSet digitizers = bHaveDigitizers ? m_pFiffDigitizerData->points : mne::FiffDigPointSet();
        m_mutex.unlock();

        if (bHaveDigitizers) {
            pHpiSettingsView->newDigitizerList(digitizers);
        }

        plControlWidgets.append(pHpiSettingsView);

        emit pluginControlWidgetsChanged(plControlWidgets, this->getName());

        m_bPluginControlWidgetsInit = true;
    }
}

//=============================================================================================================

void Hpi::updateProjections()
{
    if (m_pFiffInfo) {
        m_mutex.lock();
        if (m_iNumberBadChannels != static_cast<int>(m_pFiffInfo->bads.size()) || m_matCompProjectors.rows() == 0 || m_matCompProjectors.cols() == 0) {
            m_iNumberBadChannels = static_cast<qint16>(m_pFiffInfo->bads.size());
        } else {
            m_mutex.unlock();
            return;
        }
        m_mutex.unlock();

        Eigen::MatrixXd matProjectors = Eigen::MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());
        Eigen::MatrixXd matComp = Eigen::MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());

        if (m_bUseSSP) {
            // Use SSP + SGM + calibration
            //Do a copy here because we are going to change the activity flags of the SSP's
            FiffInfo infoTemp = *(m_pFiffInfo.get());

            //Turn on all SSP
            for (int i = 0; i < std::ssize(infoTemp.projs); ++i) {
                infoTemp.projs[i].active = true;
            }

            //Create the projector for all SSP's on
            infoTemp.make_projector(matProjectors);
            //set columns of matrix to zero depending on bad channels indexes
            for (qint32 j = 0; j < static_cast<int>(infoTemp.bads.size()); ++j) {
                matProjectors.col(mne::index_of(infoTemp.ch_names, infoTemp.bads.at(j))).setZero();
            }
        }

        if (m_bUseComp) {
            // Setup Comps
            FiffCtfComp newComp;
            //Do this always from 0 since we always read new raw data, we never actually perform a multiplication on already existing data
            if (m_pFiffInfo->make_compensator(0, 101, newComp)) {
                matComp = newComp.data.data;
            }
        }

        m_mutex.lock();
        m_matCompProjectors = matProjectors * matComp;
        m_mutex.unlock();
    }
}

//=============================================================================================================

void Hpi::onAllowedMeanErrorDistChanged(double dAllowedMeanErrorDist)
{
    m_mutex.lock();
    m_dAllowedMeanErrorDist = dAllowedMeanErrorDist * 0.001;
    m_mutex.unlock();
}

//=============================================================================================================

void Hpi::onAllowedMovementChanged(double dAllowedMovement)
{
    m_mutex.lock();
    m_dAllowedMovement = dAllowedMovement;
    m_mutex.unlock();
}

//=============================================================================================================

void Hpi::onAllowedRotationChanged(double dAllowedRotation)
{
    m_mutex.lock();
    m_dAllowedRotation = dAllowedRotation;
    m_mutex.unlock();
}

//=============================================================================================================

void Hpi::onDigitizersChanged(const mne::FiffDigPointSet& digitizers,
                              const QString& sFilePath)
{
    m_mutex.lock();
    if (m_pFiffInfo) {
        m_pFiffInfo->dig = digitizers;
    }

    m_sFilePathDigitzers = sFilePath;

    m_mutex.unlock();
}

//=============================================================================================================

void Hpi::onDoSingleHpiFit()
{
    if (m_vCoilFreqs.size() < 3) {
        QMessageBox msgBox;
        msgBox.setText("Please input HPI coil frequencies first.");
        msgBox.exec();
        return;
    }

    m_mutex.lock();
    m_bDoSingleHpi = true;
    m_mutex.unlock();
}

//=============================================================================================================

void Hpi::onDoFreqOrder()
{
    if (m_vCoilFreqs.size() < 3) {
        QMessageBox msgBox;
        msgBox.setText("Please input HPI coil frequencies first.");
        msgBox.exec();
        return;
    }

    m_mutex.lock();
    m_bDoFreqOrder = true;
    m_mutex.unlock();
}

//=============================================================================================================

void Hpi::onCoilFrequenciesChanged(const QVector<int>& vCoilFreqs)
{
    m_mutex.lock();
    m_vCoilFreqs.assign(vCoilFreqs.begin(), vCoilFreqs.end());
    m_mutex.unlock();
}

//=============================================================================================================

void Hpi::onSspStatusChanged(bool bChecked)
{
    m_bUseSSP = bChecked;
    updateProjections();
}

//=============================================================================================================

void Hpi::onCompStatusChanged(bool bChecked)
{
    m_bUseComp = bChecked;
    updateProjections();
}

//=============================================================================================================

void Hpi::onContHpiStatusChanged(bool bChecked)
{
    m_bDoContinousHpi = bChecked;
}

//=============================================================================================================

void Hpi::setFittingWindowSize(int winSize)
{
    QMutexLocker locker(&m_mutex);
    m_iFittingWindowSize = winSize;
}

//=============================================================================================================

void Hpi::onDevHeadTransAvailable(const mne::FiffCoordTrans& devHeadTrans)
{
    m_pFiffInfo->dev_head_t = devHeadTrans;
}

//=============================================================================================================

void Hpi::run()
{
    // Wait for fiff info
    bool bFiffInfo = false;

    while (!isInterruptionRequested()) {
        m_mutex.lock();
        if (m_pFiffInfo) {
            bFiffInfo = true;
        }
        m_mutex.unlock();
        if (bFiffInfo) {
            break;
        }
        msleep(100);
    }
    if (isInterruptionRequested()) {
        return;
    }

    m_mutex.lock();
    int fittingWindowSize = m_iFittingWindowSize;
    // A fit every half window keeps the head position current
    RtHpi rtHpi(m_pFiffInfo, {.coil_frequencies = m_vCoilFreqs, .window = fittingWindowSize, .hop = fittingWindowSize / 2, .max_error = m_dAllowedMeanErrorDist});
    m_mutex.unlock();
    FiffCoordTrans transDevHeadRef = rtHpi.head_transform();
    MatrixXd matData;
    bool singlePending = false;

    while (!isInterruptionRequested()) {
        if (!m_pCircularBuffer->pop(matData)) {
            continue;
        }

        m_mutex.lock();
        if (fittingWindowSize != m_iFittingWindowSize) {
            fittingWindowSize = m_iFittingWindowSize;
            Log::info("Fitting window size: {}", fittingWindowSize);
            rtHpi.set_window(fittingWindowSize, fittingWindowSize / 2);
        }
        // A single fit or a frequency ordering keeps the data coming until its fit is done
        if (m_bDoSingleHpi && !singlePending) {
            rtHpi.reset();
            singlePending = true;
        }
        if (m_bDoFreqOrder) {
            rtHpi.order_coil_frequencies();
        }
        rtHpi.set_coil_frequencies(m_vCoilFreqs);
        rtHpi.set_projectors(m_matCompProjectors);
        rtHpi.set_info(m_pFiffInfo);
        rtHpi.set_max_error(m_dAllowedMeanErrorDist);
        const double dAllowedMovement = m_dAllowedMovement;
        const double dAllowedRotation = m_dAllowedRotation;
        const std::filesystem::path digitizerFile = mne::path_from_utf8(m_sFilePathDigitzers.toStdString());
        m_mutex.unlock();

        std::optional<HpiFitResult> fitResult = rtHpi.update(matData);
        if (!fitResult || fitResult->errorDistances.empty()) {
            continue;
        }
        fitResult->digitizerFile = digitizerFile;
        // The fit assigns the frequencies to the coils when asked to
        m_mutex.lock();
        m_vCoilFreqs = rtHpi.coil_frequencies();
        m_bDoSingleHpi = false;
        m_bDoFreqOrder = false;
        m_mutex.unlock();
        singlePending = false;

        const double dMeanErrorDist = std::accumulate(fitResult->errorDistances.begin(), fitResult->errorDistances.end(), .0) / static_cast<double>(fitResult->errorDistances.size());
        emit errorsChanged(QVector<double>(fitResult->errorDistances.begin(), fitResult->errorDistances.end()), dMeanErrorDist);
        emit gofChanged(fitResult->GoF, fitResult->GoF.mean());

        if (rtHpi.accepted(*fitResult)) {
            emit devHeadTransAvailable(fitResult->devHeadTrans);

            // check for large head movement
            const double dMovement = transDevHeadRef.translation_to(fitResult->devHeadTrans.trans);
            const double dRotation = transDevHeadRef.angle_to(fitResult->devHeadTrans.trans);
            emit movementResultsChanged(dMovement, dRotation);

            fitResult->fHeadMovementDistance = static_cast<float>(dMovement);
            fitResult->fHeadMovementAngle = static_cast<float>(dRotation);
            fitResult->bIsLargeHeadMovement = dMovement > dAllowedMovement || dRotation > dAllowedRotation;
            if (fitResult->bIsLargeHeadMovement) {
                transDevHeadRef = fitResult->devHeadTrans;
            }

            m_pHpiOutput->measurementData()->setValue(*fitResult);
        }
    }
}

//=============================================================================================================

QString Hpi::getBuildInfo()
{
    return QString(HPIPLUGIN::buildDateTime()) + QString(" - ") + QString(HPIPLUGIN::buildHash());
}

//=============================================================================================================

QVariantMap Hpi::getAttributes() const
{
    QVariantMap attrs;
    attrs[QStringLiteral("filePathDigitizers")] = m_sFilePathDigitzers;
    attrs[QStringLiteral("fittingWindowSize")] = static_cast<int>(m_iFittingWindowSize);
    attrs[QStringLiteral("doFreqOrder")] = m_bDoFreqOrder;
    attrs[QStringLiteral("doContinuousHpi")] = m_bDoContinousHpi;
    attrs[QStringLiteral("useSSP")] = m_bUseSSP;
    attrs[QStringLiteral("useComp")] = m_bUseComp;
    return attrs;
}

//=============================================================================================================

void Hpi::setAttributes(const QVariantMap& attributes)
{
    if (attributes.contains(QStringLiteral("filePathDigitizers")))
        m_sFilePathDigitzers = attributes[QStringLiteral("filePathDigitizers")].toString();
    if (attributes.contains(QStringLiteral("fittingWindowSize")))
        m_iFittingWindowSize = static_cast<qint16>(attributes[QStringLiteral("fittingWindowSize")].toInt());
    if (attributes.contains(QStringLiteral("doFreqOrder")))
        m_bDoFreqOrder = attributes[QStringLiteral("doFreqOrder")].toBool();
    if (attributes.contains(QStringLiteral("doContinuousHpi")))
        m_bDoContinousHpi = attributes[QStringLiteral("doContinuousHpi")].toBool();
    if (attributes.contains(QStringLiteral("useSSP")))
        m_bUseSSP = attributes[QStringLiteral("useSSP")].toBool();
    if (attributes.contains(QStringLiteral("useComp")))
        m_bUseComp = attributes[QStringLiteral("useComp")].toBool();
}
