//=============================================================================================================
/**
 * @file     fieldline.cpp
 * @author   Juan GarciaPrieto <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel B Motta <gbmotta@mgh.harvard.edu>;
 * @since    0.1.0
 * @date     February, 2023
 *
 * @section  LICENSE
 *
 * Copyright (C) 2023, Juan G Prieto, Gabriel B Motta. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Contains the definition of the Fieldline class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================
#include <string>
#include <vector>
#include <thread>

#include "fieldline/fieldline.h"
#include "fieldline/fieldline_acqsystem.h"
#include "fieldline/fieldline_view.h"
#include "fieldline/ipfinder.h"

#include <scMeas/realtimemultisamplearray.h>
#include <utils/generics/circularbuffer.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================
#include <QLabel>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

namespace FIELDLINEPLUGIN {

//=============================================================================================================

// QSharedPointer<FIFFLIB::FiffInfo> createFiffInfo(int numChassis,
//                                                  int numChannels) {
//   QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo;
//   pFiffInfo->sfreq = 1000.0f;
//   pFiffInfo->nchan = numChassis * numChannels;
//   pFiffInfo->chs.clear();
//
//   for (int chan_i = 0; chan_i < pFiffInfo->nchan; ++chan_i) {
//     FIFFLIB::FiffChInfo channel;
//     channel.ch_name = QString("%1:%2").arg(numChannels, 2).arg(chan_i, 2);
//     channel.kind = FIFFV_MEG_CH;
//     channel.unit = FIFF_UNIT_T;
//     channel.unit_mul = FIFF_UNITM_NONE;
//     channel.chpos.coil_type = FIFFV_COIL_NONE;
//     pFiffInfo->chs.append(channel);
//     pFiffInfo->ch_names.append(channel.ch_name);
//   }
// }

// QSharedPointer<FIFFLIB::FiffInfo>
// createFiffInfo(std::vector<std::vector<int>> fl_chassis, float sfreq = 1000.f) {
//   QSharedPointer<FIFFLIB::FiffInfo> pFiffInfo;
//   pFiffInfo->sfreq = sfreq;
//   pFiffInfo->chs.clear();
//
//   int total_channels = 0;
//   int chassis_num = 0;
//   for (auto &chassis : fl_chassis) {
//     for (auto &sensor : chassis) {
//       FIFFLIB::FiffChInfo channel;
//       channel.ch_name = QString("%1:%2").arg(chassis_num, 2).arg(sensor, 2);
//       channel.kind = FIFFV_MEG_CH;
//       channel.unit = FIFF_UNIT_T;
//       channel.unit_mul = FIFF_UNITM_NONE;
//       channel.chpos.coil_type = FIFFV_COIL_NONE;
//       pFiffInfo->chs.append(channel);
//       pFiffInfo->ch_names.append(channel.ch_name);
//       ++total_channels;
//     }
//   }
//   pFiffInfo->nchan = total_channels;
//
//   return pFiffInfo;
// }
//
// QSharedPointer<FIFFLIB::FiffInfo>
// createFiffInfo(int numChassis, int numChannels, float sfreq = 1000.f) {
//   std::vector<std::vector<int>> fl;
//   for (int i = 0; i < numChassis; ++i) {
//     std::vector<int> ch(numChannels);
//     std::iota(ch.begin(), ch.end(), 1);
//     fl.push_back(std::move(ch));
//   }
//   return createFiffInfo(fl, sfreq);
// }

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Fieldline::Fieldline()
: m_pAcqSystem(nullptr)
, m_pCircularBuffer(QSharedPointer<UTILSLIB::CircularBuffer_Matrix_double>(new UTILSLIB::CircularBuffer_Matrix_double(10)))
{
  // printLog("constructor fieldline plugin");
}

//=============================================================================================================

Fieldline::~Fieldline() {
  if (this->isRunning()) {
    this->stop();
  }
}

//=============================================================================================================

QSharedPointer<SCSHAREDLIB::AbstractPlugin> Fieldline::clone() const
{
  QSharedPointer<SCSHAREDLIB::AbstractPlugin> pFieldlineClone(new Fieldline());
  return pFieldlineClone;
}

//=============================================================================================================

void Fieldline::init()
{
  printLog("Fieldline init");
  m_pCircularBuffer = QSharedPointer<UTILSLIB::CircularBuffer_Matrix_double>::create(10);
  // data infrastructure
  m_pRTMSA = SCSHAREDLIB::PluginOutputData
    <SCMEASLIB::RealTimeMultiSampleArray>::create(this, "Fieldline Plugin",
                                                  "FieldlinePlguin output");
  m_pRTMSA->measurementData()->setName(this->getName());  // Provide name to auto store widget settings
  m_outputConnectors.append(m_pRTMSA);

  m_pAcqSystem = new FieldlineAcqSystem(this);
}

//=============================================================================================================

void Fieldline::unload() {
  printLog("unload");
  delete m_pAcqSystem;
}

//=============================================================================================================

bool Fieldline::start()
{
  printLog("start Fieldline");

  initFiffInfo();

  QThread::start();
  return true;
}

void Fieldline::initFiffInfo() 
{
  QSharedPointer<FIFFLIB::FiffInfo> info;
  info->sfreq = 1000.0f;
  info->nchan = 32;
  info->chs.clear();
  for (int chan_i = 0; chan_i < info->nchan; chan_i++) {
    FIFFLIB::FiffChInfo channel;
    channel.kind = FIFFV_MEG_CH;
    channel.unit = FIFF_UNIT_T;
    channel.unit_mul = FIFF_UNITM_NONE;
    channel.chpos.coil_type = FIFFV_COIL_NONE;
    std::string channel_name(std::string("Ch. ") + std::to_string(chan_i));
    channel.ch_name = QString::fromStdString(channel_name);
    info->chs.append(channel);
    info->ch_names.append(QString::fromStdString(channel_name));
  }

  m_pRTMSA->measurementData()->initFromFiffInfo(info);
  m_pRTMSA->measurementData()->setMultiArraySize(1);
  m_pRTMSA->measurementData()->setVisibility(true);
}

//=============================================================================================================

bool Fieldline::stop() {
  printLog("stop");
  requestInterruption();
  wait(500);

  m_pRTMSA->measurementData()->clear();
  m_pCircularBuffer->clear();

  return true;
}

//=============================================================================================================

SCSHAREDLIB::AbstractPlugin::PluginType Fieldline::getType() const {
  printLog("getType Fieldline");
  return SCSHAREDLIB::AbstractPlugin::PluginType::_ISensor;
}

//=============================================================================================================

QString Fieldline::getName() const {
  return QString("Fieldline OPM");
}

//=============================================================================================================

QWidget *Fieldline::setupWidget() {
  return new FieldlineView(this);
}

//=============================================================================================================

void Fieldline::findIpAsync(std::vector<std::string>& macList,
                            std::function<void(std::vector<std::string>&)> callback) {
    std::thread ipFinder([macList, callback] {
        IPFINDER::IpFinder ipFinder;
        for (auto& mac : macList) {
            ipFinder.addMacAddress(mac);
        }
        ipFinder.findIps();
        std::vector<std::string> ipList;
        ipList.reserve(ipFinder.macIpList.size());
        for (size_t i = 0; i < ipFinder.macIpList.size(); i++) {
            ipList.emplace_back(ipFinder.macIpList[i].ip);
        }
        callback(ipList);
    });
  ipFinder.detach();
}

//=============================================================================================================

void Fieldline::run() 
{
  Eigen::MatrixXd matData;

  while (!isInterruptionRequested()) {
    if (m_pCircularBuffer->pop(matData)) {
      if (!isInterruptionRequested()) {
        m_pRTMSA->measurementData()->setValue(matData);
      }
    }
  }
}

//=============================================================================================================

QString Fieldline::getBuildInfo() {
  printLog("getBuildInfo Fieldline");
  return QString(buildDateTime()) + QString(" - ") + QString(buildHash());
}

void Fieldline::newData(double* mat, size_t numSamples, size_t numChannels) 
{
    while(!m_pCircularBuffer->push(Eigen::Map<Eigen::MatrixXd>(mat, numSamples, numChannels))) {
        printLog("Fieldline Plugin: Pushing data to circular buffer failed... Trying again.");
    }
}

}  // namespace FIELDLINEPLUGIN
