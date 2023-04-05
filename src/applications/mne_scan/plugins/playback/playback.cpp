//=============================================================================================================
/**
 * @file     playback.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>,
 *           Gabriel B Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Lorenz Esch, Gabriel B Motta. All rights reserved.
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

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using PLAYBACKPLUGIN::Playback;
using PLAYBACKPLUGIN::PlaybackSetupWidget;
using UTILSLIB::File;
using SCSHAREDLIB::AbstractSensor;
using SCSHAREDLIB::AbstractPlugin;
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

}

//=============================================================================================================

void Playback::unload()
{
}

//=============================================================================================================

bool Playback::start()
{
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
    PlaybackSetupWidget* setupWidget = new PlaybackSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}

//=============================================================================================================

void Playback::run()
{
    MatrixXd matData;
    qint32 size = 0;

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
