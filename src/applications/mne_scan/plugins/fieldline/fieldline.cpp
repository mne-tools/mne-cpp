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

#include "fieldline.h"

#include <disp/viewers/fieldlineview.h>
// #include "fieldlineproducer.h"
// #include "FormFiles/fieldlinesetup.h"

// #include <fiff/fiff.h>
// #include <scMeas/realtimemultisamplearray.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

// #include <QSettings>
#include <QDebug>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpacerItem>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

namespace FIELDLINEPLUGIN {

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Fieldline::Fieldline() {
  qDebug() << "Creating Fieldline object";
}

//=============================================================================================================

Fieldline::~Fieldline() {
  // If the program is closed while the sampling is in process
  // if(this->isRunning()) {
  //     this->stop();
  // }
  qDebug() << "Destroying Fieldline object";
}

//=============================================================================================================

QSharedPointer<SCSHAREDLIB::AbstractPlugin> Fieldline::clone() const {
  qDebug() << "Cloning Fieldline.";
  QSharedPointer<SCSHAREDLIB::AbstractPlugin> pFieldlineClone(new Fieldline());
  return pFieldlineClone;
}

//=============================================================================================================

void Fieldline::init() {

  // m_outputConnectors.append(m_pRMTSA_Natus);
  qDebug() << "Init Fieldline";
  acqSystem = std::make_unique<FieldlineAcqSystemController>();
}

//=============================================================================================================

void Fieldline::unload() { 
  qDebug() << "unload Fieldline"; 
}

//=============================================================================================================

bool Fieldline::start() {
  qDebug() << "start Fieldline";

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
  // QThread::start();
  //
  // // Start the producer
  // m_pNatusProducer =
  // QSharedPointer<NatusProducer>::create(m_iSamplesPerBlock,
  // m_iNumberChannels); m_pNatusProducer->moveToThread(&m_pProducerThread);
  // connect(m_pNatusProducer.data(), &NatusProducer::newDataAvailable,
  //         this, &Fieldline::onNewDataAvailable, Qt::DirectConnection);
  // m_pProducerThread.start();

  return true;
}

//=============================================================================================================

bool Fieldline::stop() {

  // requestInterruption();
  // wait(500);
  //
  // // Clear all data in the buffer connected to displays and other plugins
  // m_pRMTSA_Natus->measurementData()->clear();
  // m_pCircularBuffer->clear();
  //
  // m_pProducerThread.quit();
  // m_pProducerThread.wait();
  qDebug() << "Stop Fieldline";

  return true;
}

//=============================================================================================================

SCSHAREDLIB::AbstractPlugin::PluginType Fieldline::getType() const {
  qDebug() << "getType Fieldline";
  return SCSHAREDLIB::AbstractPlugin::PluginType::_ISensor;
}

//=============================================================================================================

QString Fieldline::getName() const {
  qDebug() << "getName Fieldline";
  return QString("Fieldline OPM");
}

//=============================================================================================================

QWidget *Fieldline::setupWidget() {
  qDebug() << "setupWidget Fieldline";
  guiWidget = std::make_unique<FieldlinePluginGUI>();

  // NatusSetup* widget = new NatusSetup(this);//widget is later destroyed by
  // CentralWidget - so it has to be created everytime new

  // init properties dialog
  //  widget->initGui();




  auto* frame = new QWidget();
  frame->setLayout(new QHBoxLayout());

  auto* flWidget = new DISPLIB::FieldlineView(2, 16);

  frame->layout()->addWidget(flWidget);
  flWidget->setBlinkState(0, 2, true);
  flWidget->setBlinkState(1, 5, true);

  return frame;

  // return new QLabel("Fieldline \n   OPM");
}

//=============================================================================================================

// void Fieldline::onNewDataAvailable(const Eigen::MatrixXd &matData) {
//
//     while(!m_pCircularBuffer->push(matData)) {
//         //Do nothing until the circular buffer is ready to accept new data
//         again
//     }
// }

//=============================================================================================================

void Fieldline::run() {
  qDebug() << "run Fieldline";
  // MatrixXd matData;
  //
  // while(!isInterruptionRequested()) {
  //     if(m_pCircularBuffer->pop(matData)) {
  //         //emit values
  //         if(!isInterruptionRequested()) {
  //             m_pRMTSA_Natus->measurementData()->setValue(matData);
  //         }
  //     }
  // }
}

//=============================================================================================================

QString Fieldline::getBuildInfo() {
  qDebug() << "getBuildInfo Fieldline";
  return QString(buildDateTime()) + QString(" - ") + QString(buildHash());
}

} // namespace FIELDLINEPLUGIN
