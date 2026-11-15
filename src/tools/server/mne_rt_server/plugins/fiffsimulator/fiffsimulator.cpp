//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiffsimulator.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 * @brief     Definition of the FiffSimulator class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffsimulator.h"

#include <acq/acq_device_registry.h>
#include <acq/acq_eigen.h>
#include <acq/fiff/acq_fiff_replay.h>
#include <fiff/fiff.h>
#include <fiff/fiff_types.h>
#include <base/string_utils.h>

#include <rtserver_connector/command.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
#include <QFile>
#include <QCoreApplication>
#include <QDebug>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>
#include <cstring>
#include <iostream>
#include <string>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFSIMULATORRTSERVERPLUGIN;
using namespace mne;
using namespace RTSERVER;

//=============================================================================================================
// DEFINE MEMBER CONSTANTS
//=============================================================================================================

const QString FiffSimulator::Commands::BUFSIZE = "bufsize";
const QString FiffSimulator::Commands::GETBUFSIZE = "getbufsize";
const QString FiffSimulator::Commands::ACCEL = "accel";
const QString FiffSimulator::Commands::GETACCEL = "getaccel";
const QString FiffSimulator::Commands::SIMFILE = "simfile";

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffSimulator::FiffSimulator()
: m_sResourceDataPath(QString("%1/../resources/data/MNE-sample-data/MEG/sample/sample_audvis_raw.fif").arg(QCoreApplication::applicationDirPath()))
, m_uiBufferSampleSize(200) //(4)
, m_AccelerationFactor(1.0)
, m_TrueSamplingRate(0.0)
, m_bIsRunning(false)
{
    this->init();
}

//=============================================================================================================

FiffSimulator::~FiffSimulator()
{
    stop();
}

//=============================================================================================================

void FiffSimulator::comBufsize(Command p_command)
{
    //ToDO JSON
    quint32 t_uiBuffSize = p_command.pValues()[0].toUInt();

    if (t_uiBuffSize > 0) {
        //        qInfo("bufsize %d", t_uiBuffSize);

        bool t_bWasRunning = m_bIsRunning;

        if (m_bIsRunning) {
            this->stop();
        }

        m_uiBufferSampleSize = t_uiBuffSize;

        if (t_bWasRunning)
            this->start();

        QString str = QString("\tSet %1 buffer sample size to %2 samples\r\n\n").arg(getName()).arg(t_uiBuffSize);

        m_commandManager[Commands::BUFSIZE].reply(str);
    } else {
        m_commandManager[Commands::BUFSIZE].reply("Buffer size not set\r\n");
    }
}

//=============================================================================================================

void FiffSimulator::comGetBufsize(Command p_command)
{
    bool t_bCommandIsJson = p_command.isJson();
    if (t_bCommandIsJson) {
        //
        //create JSON help object
        //
        QJsonObject t_qJsonObjectRoot;
        t_qJsonObjectRoot.insert(Commands::BUFSIZE, QJsonValue(static_cast<double>(m_uiBufferSampleSize)));
        QJsonDocument p_qJsonDocument(t_qJsonObjectRoot);

        m_commandManager[Commands::GETBUFSIZE].reply(p_qJsonDocument.toJson());
    } else {
        QString str = QString("\t%1\r\n\n").arg(m_uiBufferSampleSize);
        m_commandManager[Commands::GETBUFSIZE].reply(str);
    }
}

//=============================================================================================================

void FiffSimulator::comAccel(Command p_command)
{
    //ToDO JSON

    float t_uiAccel = p_command.pValues()[0].toFloat();

    if (t_uiAccel > 0) {
        bool t_bWasRunning = m_bIsRunning;

        if (m_bIsRunning) {
            this->stop();
        }

        m_AccelerationFactor = t_uiAccel;
        m_RawInfo.info.sfreq = m_AccelerationFactor * m_TrueSamplingRate;

        if (t_bWasRunning)
            this->start();

        QString str = QString("\tSet acceleration factor to %1\r\n\n").arg(t_uiAccel, 0, 'f', 3);

        m_commandManager[Commands::ACCEL].reply(str);
    } else
        m_commandManager[Commands::ACCEL].reply("Acceleration facor not set\r\n");
}

//=============================================================================================================

void FiffSimulator::comGetAccel(Command p_command)
{
    bool t_bCommandIsJson = p_command.isJson();
    if (t_bCommandIsJson) {
        //
        //create JSON help object
        //
        QJsonObject t_qJsonObjectRoot;
        t_qJsonObjectRoot.insert(Commands::ACCEL, QJsonValue(static_cast<double>(m_AccelerationFactor)));
        QJsonDocument p_qJsonDocument(t_qJsonObjectRoot);

        m_commandManager[Commands::GETACCEL].reply(p_qJsonDocument.toJson());
    } else {
        QString str = QString("\t%1\r\n\n").arg(m_AccelerationFactor, 0, 'f', 3);
        m_commandManager[Commands::GETACCEL].reply(str);
    }
}

//=============================================================================================================

void FiffSimulator::comSimfile(Command p_command)
{
    //
    // simulation file
    //
    QFile t_file(p_command.pValues()[0].toString());

    QString t_sResourceDataPathOld = m_sResourceDataPath;

    if (t_file.exists()) {
        m_sResourceDataPath = p_command.pValues()[0].toString();
        m_RawInfo = FiffRawData();

        if (this->readRawInfo()) {
            this->stop();

            m_commandManager[Commands::SIMFILE].reply("New simulation file set succefully.\r\n");
        } else {
            qDebug() << "Didn't set new file";
            m_sResourceDataPath = t_sResourceDataPathOld;

            m_commandManager[Commands::SIMFILE].reply("Simulation file not set.\r\n");
        }
    } else {
        qDebug() << "File does not exist on server!";
        m_sResourceDataPath = t_sResourceDataPathOld;
        m_commandManager[Commands::SIMFILE].reply("Simulation file not set.\r\n");
    }
}

//=============================================================================================================

void FiffSimulator::connectCommandManager()
{
    //Connect slots
    QObject::connect(&m_commandManager[Commands::BUFSIZE], &Command::executed, this, &FiffSimulator::comBufsize);
    QObject::connect(&m_commandManager[Commands::GETBUFSIZE], &Command::executed, this, &FiffSimulator::comGetBufsize);
    QObject::connect(&m_commandManager[Commands::ACCEL], &Command::executed, this, &FiffSimulator::comAccel);
    QObject::connect(&m_commandManager[Commands::GETACCEL], &Command::executed, this, &FiffSimulator::comGetAccel);
    QObject::connect(&m_commandManager[Commands::SIMFILE], &Command::executed, this, &FiffSimulator::comSimfile);
}

//=============================================================================================================

ConnectorID FiffSimulator::getConnectorID() const
{
    return FIFF_SIMULATOR_CONNECTOR;
}

//=============================================================================================================

const char* FiffSimulator::getName() const
{
    return "Fiff File Simulator";
}

//=============================================================================================================

void FiffSimulator::init()
{
    //
    // Read cfg file
    //
    QFile t_qFile(QString("%1/../resources/mne_rt_server/plugins/fiffsimulator/FiffSimulation.cfg").arg(QCoreApplication::applicationDirPath()));
    if (t_qFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&t_qFile);
        QString key = "simFile = ";
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.contains(key, Qt::CaseInsensitive)) {
                qint32 idx = line.indexOf(key);
                idx += key.size();

                QString sFileName = line.mid(idx, line.size() - idx);

                QFile t_qFileMeas(sFileName);

                if (t_qFileMeas.open(QIODevice::ReadOnly)) {
                    m_sResourceDataPath = sFileName;
                    qInfo() << "[FiffSimulator::init] Load simulation file " << sFileName;
                    t_qFileMeas.close();
                } else {
                    qInfo() << "[FiffSimulator::init] Trying to open simulation file " << sFileName << "read from FiffSimulation.cfg failed. Opening sample_audvis_raw.fif instead.";
                }
            }
        }
        t_qFile.close();
    }
}

//=============================================================================================================

bool FiffSimulator::start()
{
    this->init();
    if (m_RawInfo.empty() && !readRawInfo()) {
        return false;
    }
    registerFiffScheme();
    const AcqUri uri{"fiff",
                     m_RawInfo.info.filename, // the file whose info clients got
                     {{"loop", "1"},
                      {"speed", std::to_string(m_AccelerationFactor)},
                      {"block", std::to_string(m_uiBufferSampleSize)},
                      {"events", "0"}}};
    auto device = AcqDevice::open(uri.toString());
    if (!device) {
        qWarning("[FiffSimulator::start] %s", device.error().text().c_str());
        return false;
    }
    auto reader = device->stream().reader(AcqReader::ReadMode::Lossless);
    if (!reader || !device->start().ok()) {
        qWarning("[FiffSimulator::start] Cannot start the replay of %s", m_RawInfo.info.filename.c_str());
        return false;
    }
    m_device = std::move(*device);
    m_reader = std::move(*reader);
    m_bIsRunning = true;
    QThread::start();
    return true;
}

//=============================================================================================================

bool FiffSimulator::stop()
{
    m_bIsRunning = false;
    QThread::wait();
    m_reader.reset();
    m_device.reset(); // closes the replay
    return true;
}

//=============================================================================================================

void FiffSimulator::info(qint32 ID)
{
    if (m_RawInfo.empty())
        readRawInfo();

    if (!m_RawInfo.empty())
        emit remitMeasInfo(ID, m_RawInfo.info);
}

//=============================================================================================================

bool FiffSimulator::readRawInfo()
{
    if (m_RawInfo.empty()) {
        QMutexLocker locker(&mutex);

        auto raw = FiffRawData::read(m_sResourceDataPath.toStdU16String());
        if (!raw) {
            qInfo("Error: Not able to read raw info: %s", raw.error().what());
            return false;
        }
        m_RawInfo = std::move(*raw);

        m_TrueSamplingRate = m_RawInfo.info.sfreq;
        m_RawInfo.info.sfreq *= m_AccelerationFactor;
    }

    return true;
}

//=============================================================================================================

void FiffSimulator::run()
{
    // The replay paces itself at sfreq * acceleration; buffers go out calibrated, as read_raw_segment returns them.
    Eigen::MatrixXd data;
    while (m_bIsRunning) {
        const auto block = mne::read(*m_reader, m_uiBufferSampleSize, data, std::chrono::milliseconds(100));
        if (!block) {
            if (block.error().code() != AcqErrorCode::Timeout) {
                qWarning("[FiffSimulator::run] %s", block.error().text().c_str());
                return;
            }
            continue;
        }
        emit remitRawBuffer(QSharedPointer<Eigen::MatrixXf>::create(data.cast<float>()));
    }
}
