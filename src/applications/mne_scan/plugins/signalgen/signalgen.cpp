//=============================================================================================================
/**
 * @file     signalgen.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the SignalGen class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "signalgen.h"
#include "FormFiles/signalgensetupwidget.h"
#include <utils/ioutils.h>
#include <fiff/fiff_info.h>
#include <fiff/c/fiff_digitizer_data.h>
#include <scMeas/realtimemultisamplearray.h>
#include <cmath>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
#include <QtCore/QTextStream>
#include <QtCore/QFile>
#include <QMutexLocker>
#include <QList>

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SIGNALGENPLUGIN;
using namespace UTILSLIB;
using namespace SCSHAREDLIB;
using namespace UTILSLIB;
using namespace SCMEASLIB;
using namespace COMMUNICATIONLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SignalGen::SignalGen()
: numChannels(32)
, numBlocksPerSecond(5)
, sample_freq(1000.f)
, gen_freq(3)
, m_mode(Mode::Noise)
{
}

//=============================================================================================================

SignalGen::~SignalGen()
{
    if(this->isRunning()) {
        stop();
    }
}

//=============================================================================================================

void SignalGen::clear()
{
    QMutexLocker locker(&m_qMutex);
    m_pFiffInfo.reset();
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> SignalGen::clone() const
{
    QSharedPointer<SignalGen> pSignalGenClone(new SignalGen());
    return std::move(pSignalGenClone);
}

//=============================================================================================================

void SignalGen::init()
{
    m_pRTMSA_SignalGen = PluginOutputData<RealTimeMultiSampleArray>::create(this, "SignalGen", "Signal Generator Output");
    m_pRTMSA_SignalGen->measurementData()->setName(this->getName());//Provide name to auto store widget settings
    m_outputConnectors.append(m_pRTMSA_SignalGen);

    //Try to connect the cmd client on start up using localhost connection
 //   this->connectCmdClient();
}

//=============================================================================================================

void SignalGen::unload()
{
    qDebug() << "SignalGen::unload()";
}

//=============================================================================================================

bool SignalGen::start()
{
    initOutput();
    QThread::start();

    return true;
}

//=============================================================================================================

bool SignalGen::stop()
{
    // Stop this (consumer) thread first
    requestInterruption();
    wait(500);

    m_pRTMSA_SignalGen->measurementData()->clear();

    return true;
}

//=============================================================================================================

AbstractPlugin::PluginType SignalGen::getType() const
{
    return _ISensor;
}

//=============================================================================================================

QString SignalGen::getName() const
{
    return "Signal Generator";
}

//=============================================================================================================

QWidget* SignalGen::setupWidget()
{
    auto* widget = new SignalGenSetupWidget();

    connect(widget, &SignalGenSetupWidget::numChannelsChanged, [this](int nchans){this->numChannels = nchans;});
    connect(widget, &SignalGenSetupWidget::sampleFreqChanged, [this](int freq){this->sample_freq = freq;});
    connect(widget, &SignalGenSetupWidget::blocksPerSecondChanged, [this](int bps){this->numBlocksPerSecond = bps;});
    connect(widget, &SignalGenSetupWidget::genFreqChanged, [this](int freq){this->gen_freq = freq;});

    connect(widget, &SignalGenSetupWidget::modeChanged, [this](Mode m){this->m_mode = m;});

    widget->defineChannelSettings(1, 999, numChannels);
    widget->defineSampleFreqSettings(100, 10000, static_cast<int>(sample_freq));
    widget->defineBlockSettings(1, 100, numBlocksPerSecond);
    widget->defineGeneratedFreqSettings(1,999, gen_freq);

    widget->defineMode(m_mode);

    return widget;
}

//=============================================================================================================

void SignalGen::run()
{
    MatrixXd matValue;
    int samplesPerBlock = sample_freq / numBlocksPerSecond;

    matValue.resize(numChannels, samplesPerBlock);

    int sleep_time_ms = 1000 * samplesPerBlock / sample_freq;

    double time = 0;
    double pi = 2*acos(0.0);

    while(!isInterruptionRequested()) {
        switch(m_mode){
        case Mode::Zero:{
            matValue = MatrixXd::Zero(numChannels, samplesPerBlock);
            break;
        }
        case Mode::Wave: {
            for(auto i = 0; i < matValue.cols(); ++i){
                time += 1/sample_freq;
                auto value = sin(2 * pi * gen_freq * time);
                for (auto& val : matValue.col(i)){
                    val = value;
                }
            }
            matValue *= 1e-12;
            break;
        }
        case Mode::Noise:{
            matValue = MatrixXd::Random(numChannels, samplesPerBlock);
            matValue *= 1e-12;
            break;
        }
        }
        m_pRTMSA_SignalGen->measurementData()->setValue(matValue);
        msleep(sleep_time_ms);
    }
}

//=============================================================================================================

void SignalGen::initOutput()
{
    QMutexLocker locker (&m_qMutex);
    m_pFiffInfo = QSharedPointer<FIFFLIB::FiffInfo>(new FIFFLIB::FiffInfo());

    m_pFiffInfo->sfreq = sample_freq;
    m_pFiffInfo->nchan = numChannels;

    m_pFiffInfo->chs.clear();

    for (int i = 0; i< m_pFiffInfo->nchan; i++){
        FIFFLIB::FiffChInfo channel;

        channel.ch_name = "Ch. " + QString::number(i + 1);
        channel.kind = FIFFV_MEG_CH;
        channel.unit = FIFF_UNIT_T;
        channel.unit_mul = FIFF_UNITM_NONE;
        channel.chpos.coil_type = FIFFV_COIL_NONE;

        m_pFiffInfo->chs.append(channel);

        m_pFiffInfo->ch_names.append(m_pFiffInfo->chs[i].ch_name);
    }

    m_pRTMSA_SignalGen->measurementData()->initFromFiffInfo(m_pFiffInfo);
    m_pRTMSA_SignalGen->measurementData()->setMultiArraySize(1);
    m_pRTMSA_SignalGen->measurementData()->setVisibility(true);
}

//=============================================================================================================

QString SignalGen::getBuildInfo()
{
    return QString(SIGNALGENPLUGIN::buildDateTime()) + QString(" - ")  + QString(SIGNALGENPLUGIN::buildHash());
}
