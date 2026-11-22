//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     eegosports.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
 *           Johannes Vorwerk <johannes.vorwerk@umit.at>
 * @since    0.1.0
 * @date     July, 2014
 * @brief    Contains the implementation of the EEGoSports class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <base/log.h>
#include "eegosports.h"

#include <acq/devices/acq_devices.h>
#include <acq/fiff/acq_fiff.h>
#include <base/string_utils.h>
#include <channels/layoutloader.h>
#include <fiff/fiff.h>
#include <scMeas/realtimemultisamplearray.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QSettings>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Geometry>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <chrono>
#include <format>
#include <numbers>
#include <optional>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace EEGOSPORTSPLUGIN;
using namespace mne::scan;
using namespace mne;
using namespace Eigen;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

namespace
{

/** The positions of an .elc file in metres, turned by 90 degrees into RAS. */
std::optional<mne::ElectrodeLayout> readLayout(const QString& path)
{
    auto layout = mne::LayoutLoader::read_elc(mne::path_from_utf8(path.toStdString()));
    if (layout) {
        const Matrix3d rotation = AngleAxisd(std::numbers::pi / 2, Vector3d::UnitZ()).toRotationMatrix();
        for (Vector3d& r : layout->positions_3d) {
            r = rotation * r * 0.001;
        }
    }
    return layout;
}

std::optional<Vector3d> find(const mne::ElectrodeLayout& layout, const QString& name)
{
    const auto it = std::find(layout.names.begin(), layout.names.end(), name.toStdString());
    if (it == layout.names.end()) {
        return std::nullopt;
    }
    return layout.positions_3d[static_cast<std::size_t>(it - layout.names.begin())];
}

FiffDigPoint digPoint(int kind, int ident, const Vector3d& r)
{
    FiffDigPoint point;
    point.kind = kind;
    point.ident = ident;
    for (int k = 0; k < 3; ++k) {
        point.r[k] = static_cast<float>(r[k]);
    }
    return point;
}

} // anonymous namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EEGoSports::EEGoSports() = default;

//=============================================================================================================

EEGoSports::~EEGoSports()
{
    if (isRunning()) {
        stop();
    }
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> EEGoSports::clone() const
{
    return QSharedPointer<EEGoSports>(new EEGoSports());
}

//=============================================================================================================

void EEGoSports::init()
{
    m_pRMTSA_EEGoSports = PluginOutputData<RealTimeMultiSampleArray>::create(this, "EEGoSports", "EEG output data");
    m_pRMTSA_EEGoSports->measurementData()->setName(getName()); //Provide name to auto store widget settings
    m_outputConnectors.append(m_pRMTSA_EEGoSports);

    const QSettings settings("MNECPP");
    m_iSamplingFreq = settings.value("EEGOSPORTS/sFreq", 512).toInt();
    m_iSamplesPerBlock = settings.value("EEGOSPORTS/samplesPerBlock", 512).toInt();
    m_bSimulate = settings.value("EEGOSPORTS/simulate", false).toBool();
    m_sElcFilePath = settings.value("EEGOSPORTS/elcFilePath", QCoreApplication::applicationDirPath() + "/../resources/general/3DLayouts/standard_waveguard66.elc").toString();
    m_sCardinalFilePath = settings.value("EEGOSPORTS/cardinalFilePath", QString()).toString();
    m_dLPAShift = settings.value("EEGOSPORTS/LPAShift", 0.0).toDouble();
    m_dRPAShift = settings.value("EEGOSPORTS/RPAShift", 0.0).toDouble();
    m_dNasionShift = settings.value("EEGOSPORTS/NasionShift", 0.0).toDouble();
    m_sLPA = settings.value("EEGOSPORTS/LPAElectrode", QString()).toString();
    m_sRPA = settings.value("EEGOSPORTS/RPAElectrode", QString()).toString();
    m_sNasion = settings.value("EEGOSPORTS/NasionElectrode", QString()).toString();
    m_bUseTrackedCardinalMode = settings.value("EEGOSPORTS/useTrackedCardinalsMode", true).toBool();
    m_bUseElectrodeShiftMode = settings.value("EEGOSPORTS/useElectrodeshiftMode", false).toBool();
}

//=============================================================================================================

void EEGoSports::unload()
{
}

//=============================================================================================================

std::string EEGoSports::uri() const
{
    return std::format("eego://?rate={}&simulate={}", m_iSamplingFreq, m_bSimulate ? 1 : 0);
}

//=============================================================================================================

void EEGoSports::applyElectrodeLayout(FiffInfo& info) const
{
    const auto layout = readLayout(m_sElcFilePath);
    if (!layout) {
        Log::warning("[EEGoSports] Cannot read the electrode positions in {}", m_sElcFilePath.toStdString());
        return;
    }
    Vector3d center = Vector3d::Zero();
    for (const Vector3d& r : layout->positions_3d) {
        center += r;
    }
    if (!layout->positions_3d.empty()) {
        center /= static_cast<double>(layout->positions_3d.size());
    }

    // Cardinal points: tracked ones from their own file, or electrodes lowered by the set shifts
    FiffDigPointSet dig;
    const std::vector<std::tuple<int, QString, QString, double>> cardinals = {
        {FIFFV_POINT_LPA, "LPA", m_sLPA, m_dLPAShift}, {FIFFV_POINT_NASION, "Nasion", m_sNasion, m_dNasionShift}, {FIFFV_POINT_RPA, "RPA", m_sRPA, m_dRPAShift}};
    const auto tracked = m_bUseTrackedCardinalMode && !m_sCardinalFilePath.isEmpty() ? readLayout(m_sCardinalFilePath) : std::nullopt;
    for (const auto& [ident, trackedName, electrode, shift] : cardinals) {
        std::optional<Vector3d> r;
        if (tracked) {
            r = find(*tracked, trackedName);
        } else if (m_bUseElectrodeShiftMode) {
            r = find(*layout, electrode);
            if (r) {
                r->z() -= shift;
            }
        }
        if (r) {
            dig.push_back(digPoint(FIFFV_POINT_CARDINAL, ident, *r));
        } else if (tracked || m_bUseElectrodeShiftMode) {
            Log::warning("[EEGoSports] {} not found. Check the loaded layout.", ((tracked ? trackedName : electrode)).toStdString());
        }
    }

    std::size_t electrode = 0;
    for (std::size_t c = 0; c < info.chs.size() && electrode < layout->positions_3d.size(); ++c) {
        FiffChInfo& ch = info.chs[c];
        if (ch.kind != FIFFV_EEG_CH) {
            continue;
        }
        const Vector3d& r = layout->positions_3d[electrode];
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
        dig.push_back(digPoint(FIFFV_POINT_EEG, static_cast<int>(electrode), r));
        ++electrode;
    }
    info.dig = std::move(dig);
}

//=============================================================================================================

bool EEGoSports::start()
{
    mne::register_eego_scheme();
    auto device = mne::AcqDevice::open(uri());
    if (!device) {
        Log::warning("[EEGoSports::start] {}", QString::fromStdString(device.error().text()).toStdString());
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
        Log::warning("[EEGoSports::start] {}", QString::fromStdString((fiffInfo ? reader.error() : fiffInfo.error()).text()).toStdString());
        m_device.close();
        return false;
    }
    applyElectrodeLayout(*fiffInfo);

    //The user can change the channels between runs, so the measurement is set up here and not in init()
    m_pRMTSA_EEGoSports->measurementData()->initFromFiffInfo(std::make_shared<FiffInfo>(std::move(*fiffInfo)));
    m_pRMTSA_EEGoSports->measurementData()->setMultiArraySize(1);

    if (mne::AcqError e = m_device.start(); !e.ok()) {
        Log::warning("[EEGoSports::start] {}", QString::fromStdString(e.text()).toStdString());
        m_device.close();
        return false;
    }
    m_reader = std::move(*reader);
    QThread::start();
    return true;
}

//=============================================================================================================

bool EEGoSports::stop()
{
    requestInterruption();
    wait();

    m_reader = mne::AcqReader();
    m_device.close();

    if (!m_bCheckImpedances) {
        // Clear all data in the buffer connected to displays and other plugins
        m_pRMTSA_EEGoSports->measurementData()->clear();
    }
    saveSettings();
    return true;
}

//=============================================================================================================

void EEGoSports::saveSettings() const
{
    QSettings settings("MNECPP");
    settings.setValue("EEGOSPORTS/sFreq", m_iSamplingFreq);
    settings.setValue("EEGOSPORTS/samplesPerBlock", m_iSamplesPerBlock);
    settings.setValue("EEGOSPORTS/simulate", m_bSimulate);
    settings.setValue("EEGOSPORTS/LPAShift", m_dLPAShift);
    settings.setValue("EEGOSPORTS/RPAShift", m_dRPAShift);
    settings.setValue("EEGOSPORTS/NasionShift", m_dNasionShift);
    settings.setValue("EEGOSPORTS/LPAElectrode", m_sLPA);
    settings.setValue("EEGOSPORTS/RPAElectrode", m_sRPA);
    settings.setValue("EEGOSPORTS/NasionElectrode", m_sNasion);
    settings.setValue("EEGOSPORTS/elcFilePath", m_sElcFilePath);
    settings.setValue("EEGOSPORTS/cardinalFilePath", m_sCardinalFilePath);
    settings.setValue("EEGOSPORTS/useTrackedCardinalsMode", m_bUseTrackedCardinalMode);
    settings.setValue("EEGOSPORTS/useElectrodeshiftMode", m_bUseElectrodeShiftMode);
}

//=============================================================================================================

AbstractPlugin::PluginType EEGoSports::getType() const
{
    return _ISensor;
}

//=============================================================================================================

QString EEGoSports::getName() const
{
    return "EEGoSports";
}

//=============================================================================================================

QWidget* EEGoSports::setupWidget()
{
    auto* widget = new EEGoSportsSetupWidget(this); //widget is later destroyed by CentralWidget - so it has to be created everytime new
    widget->initGui();
    return widget;
}

//=============================================================================================================

void EEGoSports::onUpdateCardinalPoints(const QString& sLPA, double dLPA, const QString& sRPA, double dRPA, const QString& sNasion, double dNasion)
{
    m_dLPAShift = dLPA;
    m_dRPAShift = dRPA;
    m_dNasionShift = dNasion;
    m_sLPA = sLPA;
    m_sRPA = sRPA;
    m_sNasion = sNasion;
}

//=============================================================================================================

void EEGoSports::showImpedanceDialog()
{
    // Open Impedance dialog only if no sampling process is active
    if (!isRunning()) {
        if (m_pEEGoSportsImpedanceWidget == nullptr) {
            m_pEEGoSportsImpedanceWidget = QSharedPointer<EEGoSportsImpedanceWidget>(new EEGoSportsImpedanceWidget(this));
        }
        if (!m_pEEGoSportsImpedanceWidget->isVisible()) {
            m_pEEGoSportsImpedanceWidget->setWindowTitle("EEGoSports - Measure impedances");
            m_pEEGoSportsImpedanceWidget->show();
            m_pEEGoSportsImpedanceWidget->raise();
        }
        m_pEEGoSportsImpedanceWidget->initGraphicScene();
    }
}

//=============================================================================================================

void EEGoSports::showSetupProjectDialog()
{
    if (m_pEEGoSportsSetupProjectWidget == nullptr) {
        m_pEEGoSportsSetupProjectWidget = QSharedPointer<EEGoSportsSetupProjectWidget>(new EEGoSportsSetupProjectWidget(this));
        connect(m_pEEGoSportsSetupProjectWidget.data(), &EEGoSportsSetupProjectWidget::cardinalPointsChanged, this, &EEGoSports::onUpdateCardinalPoints);
    }
    if (!m_pEEGoSportsSetupProjectWidget->isVisible()) {
        m_pEEGoSportsSetupProjectWidget->setWindowTitle("EEGoSports EEG Connector - Setup project");
        m_pEEGoSportsSetupProjectWidget->show();
        m_pEEGoSportsSetupProjectWidget->raise();
    }
}

//=============================================================================================================

void EEGoSports::run()
{
    if (m_bCheckImpedances) {
        while (!isInterruptionRequested()) {
            const auto impedances = m_device.measureImpedances(std::chrono::seconds(5));
            if (!impedances) {
                Log::warning("[EEGoSports::run] {}", QString::fromStdString(impedances.error().text()).toStdString());
                return;
            }
            // Electrode i of the cap is referential input i, and those lead the stream
            VectorXd ohms = VectorXd::Zero(static_cast<Index>(impedances->size()));
            for (const mne::AcqImpedance& impedance : *impedances) {
                if (impedance.channel < impedances->size()) {
                    ohms[static_cast<Index>(impedance.channel)] = impedance.ohms;
                }
            }
            if (m_pEEGoSportsImpedanceWidget) {
                EEGoSportsImpedanceWidget* widget = m_pEEGoSportsImpedanceWidget.data();
                QMetaObject::invokeMethod(widget, [widget, ohms] { widget->updateGraphicScene(ohms); }, Qt::QueuedConnection);
            }
            QThread::msleep(200);
        }
        return;
    }

    const std::size_t channels = m_device.stream().info()->channelCount();
    MatrixXd block(static_cast<Index>(channels), m_iSamplesPerBlock);
    while (!isInterruptionRequested()) {
        const auto read = m_reader.read(mne::AcqMatrixRef<double>::columnMajor(block.data(), channels, static_cast<std::size_t>(block.cols())),
                                        std::chrono::milliseconds(100));
        if (!read) {
            if (read.error() == mne::AcqErrorCode::Timeout || read.error() == mne::AcqErrorCode::Overrun) {
                continue;
            }
            Log::warning("[EEGoSports::run] {}", QString::fromStdString(read.error().text()).toStdString());
            return;
        }
        m_pRMTSA_EEGoSports->measurementData()->setValue(block);
    }
}

//=============================================================================================================

QString EEGoSports::getBuildInfo()
{
    return QString(EEGOSPORTSPLUGIN::buildDateTime()) + QString(" - ") + QString(EEGOSPORTSPLUGIN::buildHash());
}
