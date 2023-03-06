//=============================================================================================================
/**
 * @file     randomdata.cpp
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
 * @brief    Contains the definition of the RandomData class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <utility>

#include "randomdata/randomdata.h"

#include <fiff/fiff_info.h>
#include <scMeas/realtimemultisamplearray.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSettings>
#include <QDebug>
#include <QLabel>
#include <numeric>
#include <vector>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================


//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

namespace RANDOMDATAPLUGIN {

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

RandomData::RandomData()
: m_pFiffInfo(QSharedPointer<FIFFLIB::FiffInfo>(new FIFFLIB::FiffInfo())) 
{
  m_pRTMSA_RandomData = SCSHAREDLIB::PluginOutputData<
      SCMEASLIB::RealTimeMultiSampleArray>::create(this, "RandomData Out",
                                                   "RandomDataPlguin output");
  m_pRTMSA_RandomData->measurementData()->setName(this->getName());
  m_outputConnectors.append(m_pRTMSA_RandomData);
}

//=============================================================================================================

RandomData::~RandomData()
{
   if(this->isRunning()) {
       RandomData::stop();
   }
}

//=============================================================================================================

QSharedPointer<SCSHAREDLIB::AbstractPlugin> RandomData::clone() const
{
  QSharedPointer<SCSHAREDLIB::AbstractPlugin> pRandomDataClone(new RandomData());
  return pRandomDataClone;
}

//=============================================================================================================

void RandomData::init()
{
  m_pFiffInfo->sfreq = 1000.0f;
  m_pFiffInfo->nchan = 32;
  m_pFiffInfo->chs.clear();

  for (int chan_i = 0; chan_i < m_pFiffInfo->nchan; ++chan_i) {
    FIFFLIB::FiffChInfo channel;
    channel.ch_name = "Ch. " + QString::number(chan_i);
    channel.kind = FIFFV_MEG_CH;
    channel.unit = FIFF_UNIT_T;
    channel.unit_mul = FIFF_UNITM_NONE;
    channel.chpos.coil_type = FIFFV_COIL_NONE;
    m_pFiffInfo->chs.append(channel);
    m_pFiffInfo->ch_names.append(channel.ch_name);
  }

  m_pRTMSA_RandomData->measurementData()->initFromFiffInfo(
      m_pFiffInfo);  //     if(m_pCircularBuffer->pop(matData)) {
  m_pRTMSA_RandomData->measurementData()->setMultiArraySize(1);
  m_pRTMSA_RandomData->measurementData()->setVisibility(true);

  matData.resize(m_pFiffInfo->nchan, 200);

  qDebug() << " ^^^^^^^^^^^^^^^^^^^^ Init Random Data  (....again)";
}

//=============================================================================================================

void RandomData::unload()
{
}

//=============================================================================================================

bool RandomData::start()
{
  // Init circular buffer to transmit data from the producer to this thread
  // if(!m_pCircularBuffer) {
  //     m_pCircularBuffer = QSharedPointer<CircularBuffer_Matrix_double>(new
  //     CircularBuffer_Matrix_double(10));
  // }
  //
  // //Setup fiff info before setting up the RMTSA because we need it to init
  // the RTMSA setUpFiffInfo();
  //
  // //Set the channel size of the RMTSA - this needs to be done here and NOT in
  // the init() function because the user can change the number of channels
  // during runtime
  // m_pRMTSA_Natus->measurementData()->initFromFiffInfo(m_pFiffInfo);
  // m_pRMTSA_Natus->measurementData()->setMultiArraySize(1);
  //
  QThread::start();
  //
  // // Start the producer
  // m_pNatusProducer =
  // QSharedPointer<NatusProducer>::create(m_iSamplesPerBlock,
  // m_iNumberChannels); m_pNatusProducer->moveToThread(&m_pProducerThread);
  // connect(m_pNatusProducer.data(), &NatusProducer::newDataAvailable,
  //         this, &RandomData::onNewDataAvailable, Qt::DirectConnection);
  // m_pProducerThread.start();

  return true;
}

//=============================================================================================================

bool RandomData::stop()
{
  requestInterruption();
  wait(500);
  m_pRTMSA_RandomData->measurementData()->clear();

  return true;
}

//=============================================================================================================

SCSHAREDLIB::AbstractPlugin::PluginType RandomData::getType() const
{
  return SCSHAREDLIB::AbstractPlugin::PluginType::_ISensor;
}

//=============================================================================================================

QString RandomData::getName() const
{
  return "Random Data";
}

//=============================================================================================================

QWidget* RandomData::setupWidget()
{

  // guiWidget = std::make_unique<RandomDataPluginGUI>();

  // NatusSetup* widget = new NatusSetup(this);//widget is later destroyed by
  // CentralWidget - so it has to be created everytime new

  // init properties dialog
  //  widget->initGui();

  // auto *frame = new QWidget();
  // frame->setLayout(new QHBoxLayout());
  //
  // auto *flWidget = new DISPLIB::RandomDataView(2, 16);
  //
  // frame->layout()->addWidget(flWidget);
  // flWidget->setBlinkState(0, 2, true);
  // flWidget->setBlinkState(1, 5, true);

  return new QLabel("Random data Baby!");
}

//=============================================================================================================

void RandomData::run() {

  for (;;) {
    // gather the data

    matData = Eigen::MatrixXd::Random(m_pFiffInfo->nchan, 200);
    matData *= 4e-12;

    msleep(200);

    if (isInterruptionRequested())
      break;
    m_pRTMSA_RandomData->measurementData()->setValue(matData);
  }
}

//=============================================================================================================

QString RandomData::getBuildInfo() {
  return QString(buildDateTime()) + QString(" - ") + QString(buildHash());
}

}  // namespace RANDOMDATAPLUGIN

