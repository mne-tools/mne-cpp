//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     tmsi.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     September, 2013
 * @brief    Contains the implementation of the TMSI class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "tmsi.h"

#include <acq/devices/acq_devices.h>
#include <acq/fiff/acq_fiff.h>
#include <base/string_utils.h>
#include <channels/layoutloader.h>
#include <fiff/fiff.h>
#include <scMeas/realtimemultisamplearray.h>

#if defined(_WIN32)
#include <windows.h>
#endif

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QApplication>
#include <QDebug>
#include <QThreadPool>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Geometry>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>
#include <format>
#include <numbers>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace TMSIPLUGIN;
using namespace mne::scan;
using namespace mne;
using namespace Eigen;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

namespace
{

constexpr int kBeepCode = 252;

void beep()
{
#if defined(_WIN32)
    QThreadPool::globalInstance()->start([] { Beep(450, 700); });
#else
    QMetaObject::invokeMethod(qApp, [] { QApplication::beep(); }, Qt::QueuedConnection);
#endif
}

} // anonymous namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TMSI::TMSI()
{
    // Create record file option action bar item/button
    m_pActionSetupProject = new QAction(QIcon(":/images/database.png"), tr("Setup project"), this);
    m_pActionSetupProject->setStatusTip(tr("Setup project"));
    connect(m_pActionSetupProject, &QAction::triggered, this, &TMSI::showSetupProjectDialog);
    addPluginAction(m_pActionSetupProject);

    // Create impedance action bar item/button
    m_pActionImpedance = new QAction(QIcon(":/images/impedances.png"), tr("Check impedance values"), this);
    m_pActionImpedance->setStatusTip(tr("Check impedance values"));
    connect(m_pActionImpedance, &QAction::triggered, this, &TMSI::showImpedanceDialog);
    addPluginAction(m_pActionImpedance);
}

//=============================================================================================================

TMSI::~TMSI()
{
    //If the program is closed while the sampling is in process
    if (this->isRunning()) {
        this->stop();
    }
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> TMSI::clone() const
{
    QSharedPointer<TMSI> pTMSIClone(new TMSI());
    return pTMSIClone;
}

//=============================================================================================================

void TMSI::init()
{
    m_pRMTSA_TMSI = PluginOutputData<RealTimeMultiSampleArray>::create(this, "TMSI", "EEG output data");
    m_pRMTSA_TMSI->measurementData()->setName(this->getName()); //Provide name to auto store widget settings

    m_outputConnectors.append(m_pRMTSA_TMSI);

    m_sElcFilePath = QCoreApplication::applicationDirPath() + "/../resources/general/3DLayouts/standard_waveguard128_duke.elc";
}

//=============================================================================================================

void TMSI::unload()
{
}

//=============================================================================================================

std::string TMSI::uri() const
{
    return std::format("tmsi://?rate={}&channels={}&average={}&simulate={}", m_iSamplingFreq, m_iNumberOfChannels,
                       m_bUseCommonAverage ? 1 : 0, m_bSimulate ? 1 : 0);
}

//=============================================================================================================

void TMSI::applyElectrodeLayout(FiffInfo& info) const
{
    // Only works for ANT Neuro Waveguard Duke caps
    const auto layout = mne::LayoutLoader::read_elc(mne::pathFromUtf8(m_sElcFilePath.toStdString()));
    if (!layout) {
        qWarning() << "[TMSI] Cannot read the electrode positions in" << m_sElcFilePath;
        return;
    }

    // The .elc positions are in mm and turned by 90 degrees against RAS
    const Matrix3d rotation = AngleAxisd(std::numbers::pi / 2, Vector3d::UnitZ()).toRotationMatrix();
    std::vector<Vector3d> positions;
    Vector3d center = Vector3d::Zero();
    for (const Vector3d& r : layout->positions_3d) {
        positions.push_back(rotation * r * 0.001);
        center += positions.back();
    }
    if (!positions.empty()) {
        center /= static_cast<double>(positions.size());
    }

    FiffDigPointSet dig;
    // Fiducials approximated from the electrodes next to them: LE2, Z1, RE2 lowered by 1, 6 and 1 cm
    const std::vector<std::tuple<const char*, int, double>> cardinals = {{"LE2", FIFFV_POINT_LPA, 0.01},
                                                                         {"Z1", FIFFV_POINT_NASION, 0.06},
                                                                         {"RE2", FIFFV_POINT_RPA, 0.01}};
    for (const auto& [name, ident, drop] : cardinals) {
        const auto it = std::find(layout->names.begin(), layout->names.end(), name);
        if (it == layout->names.end()) {
            qWarning() << "[TMSI]" << name << "not found. Check the loaded layout.";
            continue;
        }
        FiffDigPoint point;
        point.kind = FIFFV_POINT_CARDINAL;
        point.ident = ident;
        const Vector3d r = positions[static_cast<std::size_t>(it - layout->names.begin())] - Vector3d(0.0, 0.0, drop);
        for (int k = 0; k < 3; ++k) {
            point.r[k] = static_cast<float>(r[k]);
        }
        dig.push_back(point);
    }

    std::size_t electrode = 0;
    for (std::size_t c = 0; c < info.chs.size() && electrode < positions.size(); ++c) {
        FiffChInfo& ch = info.chs[c];
        if (ch.kind != FIFFV_EEG_CH) {
            continue;
        }
        const Vector3d& r = positions[electrode];
        ch.ch_name = "EEG " + layout->names[electrode];
        info.ch_names[c] = ch.ch_name;
        ch.coord_frame = FIFFV_COORD_HEAD;
        ch.chpos.coil_type = FIFFV_COIL_EEG;
        for (int k = 0; k < 3; ++k) {
            ch.eeg_loc(k, 0) = r[k];
            ch.eeg_loc(k, 1) = center[k];
            ch.chpos.r0[k] = static_cast<float>(r[k]);
            ch.chpos.ex[k] = static_cast<float>(center[k]);
        }
        ch.chpos.ey << 0.0f, 1.0f, 0.0f;
        ch.chpos.ez << 0.0f, 0.0f, 1.0f;

        FiffDigPoint point;
        point.kind = FIFFV_POINT_EEG;
        point.ident = static_cast<int>(electrode);
        for (int k = 0; k < 3; ++k) {
            point.r[k] = static_cast<float>(r[k]);
        }
        dig.push_back(point);
        ++electrode;
    }
    info.dig = std::move(dig);
}

//=============================================================================================================

bool TMSI::start()
{
    mne::registerTmsiScheme();
    auto device = mne::AcqDevice::open(uri());
    if (!device) {
        qWarning() << "[TMSI::start]" << QString::fromStdString(device.error().text());
        return false;
    }
    m_device = std::move(*device);
    if (m_bCheckImpedances) {
        QThread::start();
        return true;
    }

    auto fiffInfo = mne::toFiffInfo(*m_device.stream().info());
    auto reader = m_device.stream().reader(mne::AcqReader::ReadMode::Lossless);
    if (!fiffInfo || !reader) {
        qWarning() << "[TMSI::start]" << QString::fromStdString((fiffInfo ? reader.error() : fiffInfo.error()).text());
        m_device.close();
        return false;
    }
    applyElectrodeLayout(*fiffInfo);
    m_iStimRow = -1;
    for (std::size_t c = 0; c < fiffInfo->chs.size() && m_iStimRow < 0; ++c) {
        if (fiffInfo->chs[c].kind == FIFFV_STIM_CH) {
            m_iStimRow = static_cast<int>(c);
        }
    }

    //The user can change the channels between runs, so the measurement is set up here and not in init()
    m_pRMTSA_TMSI->measurementData()->initFromFiffInfo(std::make_shared<FiffInfo>(std::move(*fiffInfo)));
    m_pRMTSA_TMSI->measurementData()->setMultiArraySize(1);

    if (mne::AcqError e = m_device.start(); !e.ok()) {
        qWarning() << "[TMSI::start]" << QString::fromStdString(e.text());
        m_device.close();
        return false;
    }
    m_reader = std::move(*reader);

    if (m_bBeepTrigger) {
        m_qTimerTrigger.start();
    }
    m_pTmsiManualAnnotationWidget = QSharedPointer<TMSIManualAnnotationWidget>(new TMSIManualAnnotationWidget(this));
    if (m_bUseKeyboardTrigger) {
        m_pTmsiManualAnnotationWidget->initGui();
        m_pTmsiManualAnnotationWidget->show();
    }

    QThread::start();
    return true;
}

//=============================================================================================================

bool TMSI::stop()
{
    requestInterruption();
    wait();

    m_reader = mne::AcqReader();
    m_device.close();

    // Clear all data in the buffer connected to displays and other plugins
    m_pRMTSA_TMSI->measurementData()->clear();

    if (m_pTmsiManualAnnotationWidget) {
        m_pTmsiManualAnnotationWidget->hide();
    }

    return true;
}

//=============================================================================================================

AbstractPlugin::PluginType TMSI::getType() const
{
    return _ISensor;
}

//=============================================================================================================

QString TMSI::getName() const
{
    return "TMSI";
}

//=============================================================================================================

QWidget* TMSI::setupWidget()
{
    TMSISetupWidget* widget = new TMSISetupWidget(this); //widget is later destroyed by CentralWidget - so it has to be created everytime new

    //init properties dialog
    widget->initGui();

    return widget;
}

//=============================================================================================================

void TMSI::setKeyboardTriggerType(int type)
{
    m_iTriggerType = type;
}

//=============================================================================================================

void TMSI::run()
{
    const std::size_t channels = m_device.stream().info()->channelCount();

    if (m_bCheckImpedances) {
        while (!isInterruptionRequested()) {
            const auto impedances = m_device.measureImpedances(std::chrono::seconds(5));
            if (!impedances) {
                qWarning() << "[TMSI::run]" << QString::fromStdString(impedances.error().text());
                return;
            }
            // Electrode i of the cap is ExG input i, and the ExG inputs lead the stream
            VectorXd ohms = VectorXd::Zero(static_cast<Index>(impedances->size()));
            for (const mne::AcqImpedance& impedance : *impedances) {
                if (impedance.channel < impedances->size()) {
                    ohms[static_cast<Index>(impedance.channel)] = impedance.ohms;
                }
            }
            if (m_pTmsiImpedanceWidget) {
                TMSIImpedanceWidget* widget = m_pTmsiImpedanceWidget.data();
                QMetaObject::invokeMethod(widget, [widget, ohms] { widget->updateGraphicScene(ohms); }, Qt::QueuedConnection);
            }
            QThread::msleep(200);
        }
        return;
    }

    MatrixXd block(static_cast<Index>(channels), m_iSamplesPerBlock);
    while (!isInterruptionRequested()) {
        const auto read = m_reader.read(mne::AcqMatrixRef<double>::columnMajor(block.data(), channels, static_cast<std::size_t>(block.cols())),
                                        std::chrono::milliseconds(100));
        if (!read) {
            if (read.error() == mne::AcqErrorCode::Timeout || read.error() == mne::AcqErrorCode::Overrun) {
                continue;
            }
            qWarning() << "[TMSI::run]" << QString::fromStdString(read.error().text());
            return;
        }
        if (m_iStimRow >= 0) {
            // Triggers of this computer go to the last sample of the block, where the subject sees them
            if (m_bBeepTrigger && m_qTimerTrigger.elapsed() >= m_iTriggerInterval) {
                beep();
                block(m_iStimRow, block.cols() - 1) = kBeepCode;
                m_qTimerTrigger.restart();
            }
            if (const int code = m_iTriggerType.exchange(0); m_bUseKeyboardTrigger && code != 0) {
                block(m_iStimRow, block.cols() - 1) = code;
            }
        }
        m_pRMTSA_TMSI->measurementData()->setValue(block);
    }
}

//=============================================================================================================

void TMSI::showImpedanceDialog()
{
    // Open Impedance dialog only if no sampling process is active
    if (!this->isRunning()) {
        if (m_pTmsiImpedanceWidget == nullptr) {
            m_pTmsiImpedanceWidget = QSharedPointer<TMSIImpedanceWidget>(new TMSIImpedanceWidget(this));
        }

        if (!m_pTmsiImpedanceWidget->isVisible()) {
            m_pTmsiImpedanceWidget->setWindowTitle("MNE Scan - Measure impedances");
            m_pTmsiImpedanceWidget->show();
            m_pTmsiImpedanceWidget->raise();
        }

        m_pTmsiImpedanceWidget->initGraphicScene();
    }
}

//=============================================================================================================

void TMSI::showSetupProjectDialog()
{
    // Open setup project widget
    if (m_pTmsiSetupProjectWidget == nullptr) {
        m_pTmsiSetupProjectWidget = QSharedPointer<TMSISetupProjectWidget>(new TMSISetupProjectWidget(this));
    }

    if (!m_pTmsiSetupProjectWidget->isVisible()) {
        m_pTmsiSetupProjectWidget->setWindowTitle("TMSI EEG Connector - Setup project");
        m_pTmsiSetupProjectWidget->initGui();
        m_pTmsiSetupProjectWidget->show();
        m_pTmsiSetupProjectWidget->raise();
    }
}

//=============================================================================================================

QString TMSI::getBuildInfo()
{
    return QString(TMSIPLUGIN::buildDateTime()) + QString(" - ") + QString(TMSIPLUGIN::buildHash());
}
