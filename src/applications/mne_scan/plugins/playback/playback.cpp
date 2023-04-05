//=============================================================================================================
/**
 * @file     playback.cpp
 * @author   Gabriel B Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.9
 * @date     April, 2023
 *
 * @section  LICENSE
 *
 * Copyright (C) 2023, Gabriel B Motta. All rights reserved.
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
 * @brief    Definition of the Playback class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "playback.h"

#include "FormFiles/playbacksetupwidget.h"

#include <scMeas/realtimemultisamplearray.h>
#include <fiff/fiff_stream.h>
#include <utils/file.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFileDialog>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using PLAYBACKPLUGIN::Playback;
using PLAYBACKPLUGIN::PlaybackSetupWidget;
using UTILSLIB::File;
using FIFFLIB::FiffStream;
using SCSHAREDLIB::AbstractSensor;
using SCSHAREDLIB::AbstractPlugin;
using SCSHAREDLIB::PluginOutputData;
using SCMEASLIB::RealTimeMultiSampleArray;
using Eigen::MatrixXd;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Playback::Playback()
{
}

//=============================================================================================================

Playback::~Playback()
{
    if(this->isRunning()) {
        stop();
    }
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> Playback::clone() const
{
    QSharedPointer<AbstractPlugin> pPlaybackClone(new Playback);
    return pPlaybackClone;
}

//=============================================================================================================

void Playback::init()
{
    m_pRTMSA_Playback = PluginOutputData<RealTimeMultiSampleArray>::create(this, "Playback", "Playback Output");
    m_pRTMSA_Playback->measurementData()->setName(this->getName());//Provide name to auto store widget settings
    m_outputConnectors.append(m_pRTMSA_Playback);
}

//=============================================================================================================

void Playback::unload()
{
}

//=============================================================================================================

bool Playback::start()
{
    if(sourceFilePath.isEmpty()){
        qWarning() << "[Playback::start] Source path empty";
        return false;
    }

    sourceFile = std::make_unique<QFile>(sourceFilePath);

    if(!sourceFile->exists()){
        qWarning() << "[Playback::start] Could not find:" << sourceFilePath;
        return false;
    }

    if (!sourceFile->open(QIODevice::ReadOnly)){
        qWarning() << "[Playback::start] Unable to open:" << sourceFilePath;
        return false;
    }

    if (!loadFiffRawData()){
        qWarning() << "[Playback::start] Not able to read raw info or data from:" << sourceFilePath;
        return false;
    }

    QThread::start();

    return true;
}

//=============================================================================================================

bool Playback::stop()
{
    requestInterruption();
    wait();

    m_bPluginControlWidgetsInit = false;

    return true;
}

//=============================================================================================================

AbstractPlugin::PluginType Playback::getType() const
{
    return _ISensor;
}

//=============================================================================================================

QString Playback::getName() const
{
    return "Playback";
}

//=============================================================================================================

QWidget* Playback::setupWidget()
{
    PlaybackSetupWidget* widget = new PlaybackSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new

    connect(widget, &PlaybackSetupWidget::newSourceFileSet,
            this, &Playback::setSourceFile, Qt::UniqueConnection);

    return widget;
}

//=============================================================================================================

void Playback::run()
{
    int first = rawData.first_samp;
    int last = rawData.last_samp;
    int bufferSize = rawData.info.sfreq;
    MatrixXd data;
    MatrixXd times;

    while(!isInterruptionRequested()) {
        if(m_pCircularBuffer) {

        }
    }
}

//=============================================================================================================

QString Playback::getBuildInfo()
{
    return QString(buildDateTime()) + QString(" - ")  + QString(buildHash());
}

//=============================================================================================================

void Playback::setSourceFile(QString filePath)
{
    sourceFilePath = filePath;
    qDebug() << "SET:" << sourceFilePath;
}

//=============================================================================================================

bool Playback::loadFiffRawData()
{
    if(!FiffStream::setup_read_raw(*sourceFile, rawData))
    {
        rawData.clear();
        return false;
    }
    return true;
}

