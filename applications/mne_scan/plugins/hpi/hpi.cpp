//=============================================================================================================
/**
 * @file     hpi.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 *           Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Lorenz Esch, Ruben Dörfel. All rights reserved.
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
 * @brief    Definition of the Hpi class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>

#include "hpi.h"

#include "FormFiles/hpisetupwidget.h"

#include <disp/viewers/hpisettingsview.h>
#include <scMeas/realtimemultisamplearray.h>
#include <scMeas/realtimehpiresult.h>
#include <inverse/hpiFit/hpifit.h>
#include <inverse/hpiFit/hpidataupdater.h>

#include <fiff/fiff_info.h>
#include <fiff/c/fiff_digitizer_data.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDir>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace HPIPLUGIN;
using namespace SCMEASLIB;
using namespace UTILSLIB;
using namespace DISPLIB;
using namespace FIFFLIB;
using namespace SCSHAREDLIB;
using namespace Eigen;
using namespace INVERSELIB;

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
, m_pCircularBuffer(CircularBuffer_Matrix_double::SPtr::create(40))
{
    connect(this, &Hpi::devHeadTransAvailable,
            this, &Hpi::onDevHeadTransAvailable, Qt::BlockingQueuedConnection);
}

//=============================================================================================================

Hpi::~Hpi()
{
    if(isRunning()) {
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
    m_pHpiOutput->measurementData()->setName(this->getName());//Provide name to auto store widget settings
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

    m_pFiffInfo.clear();
    m_pFiffDigitizerData.clear();
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
    HpiSetupWidget* setupWidget = new HpiSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}

//=============================================================================================================

void Hpi::update(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    if(QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<RealTimeMultiSampleArray>()) {
        //Check if the fiff info was inititalized

        manageInitialization(pRTMSA);

        // Check if data is present
        if(pRTMSA->getMultiSampleArray().size() > 0) {
            //If bad channels changed, recalcluate projectors
            updateProjections();

            m_mutex.lock();
            bool bDoSingleHpi = m_bDoSingleHpi;
            bool bDoFreqOrder = m_bDoFreqOrder;
            m_mutex.unlock();

            if(bDoFreqOrder || bDoSingleHpi) {
                while(!m_pCircularBuffer->push(pRTMSA->getMultiSampleArray()[0])) {
                    //Do nothing until the circular buffer is ready to accept new data again
                }
            }

            if(m_bDoContinousHpi && (m_vCoilFreqs.size() >= 3)) {
                for(unsigned char i = 0; i < pRTMSA->getMultiSampleArray().size(); ++i) {
                    // Please note that we do not need a copy here since this function will block until
                    // the buffer accepts new data again. Hence, the data is not deleted in the actual
                    // Measurement function after it emitted the notify signal.
                    while(!m_pCircularBuffer->push(pRTMSA->getMultiSampleArray()[i])) {
                        //Do nothing until the circular buffer is ready to accept new data again
                    }
                }
            }
        }
    }
}

//=============================================================================================================

void Hpi::manageInitialization(QSharedPointer<SCMEASLIB::RealTimeMultiSampleArray> pRTMSA)
{
    if(!m_pFiffInfo) {
        initFiffInfo(pRTMSA->info());
    }
    if(!m_bPluginControlWidgetsInit) {
        initPluginControlWidgets();
    }
    if(!m_pFiffDigitizerData && m_pFiffInfo){
        initFiffDigitizers(pRTMSA->digitizerData());
    }
}

//=============================================================================================================

void Hpi::initFiffInfo(QSharedPointer<FIFFLIB::FiffInfo> info)
{
    m_mutex.lock();
    m_pFiffInfo = info;
    m_pHpiOutput->measurementData()->setFiffInfo(m_pFiffInfo);
    m_mutex.unlock();
    updateProjections();
}

//=============================================================================================================

void Hpi::initFiffDigitizers(QSharedPointer<FIFFLIB::FiffDigitizerData> fiffDig)
{
    if(fiffDig){
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
    if(m_pFiffInfo) {
        bFiffInfo = true;
    }
    m_mutex.unlock();

    if(bFiffInfo) {
        QList<QWidget*> plControlWidgets;

        // Projects Settings
        HpiSettingsView* pHpiSettingsView = new HpiSettingsView(QString("MNESCAN/%1/").arg(this->getName()));

        pHpiSettingsView->loadCoilPresets(QCoreApplication::applicationDirPath() + "/mne_scan_plugins/hpi.json");

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

        plControlWidgets.append(pHpiSettingsView);

        emit pluginControlWidgetsChanged(plControlWidgets, this->getName());

        m_bPluginControlWidgetsInit = true;
    }
}

//=============================================================================================================

void Hpi::updateProjections()
{
    if(m_pFiffInfo) {
        m_mutex.lock();
        if(m_iNumberBadChannels != m_pFiffInfo->bads.size()
           || m_matCompProjectors.rows() == 0
           || m_matCompProjectors.cols() == 0) {
            m_iNumberBadChannels = m_pFiffInfo->bads.size();
        } else {
            m_mutex.unlock();
            return;
        }
        m_mutex.unlock();

        Eigen::MatrixXd matProjectors = Eigen::MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());
        Eigen::MatrixXd matComp = Eigen::MatrixXd::Identity(m_pFiffInfo->chs.size(), m_pFiffInfo->chs.size());

        if(m_bUseSSP) {
            // Use SSP + SGM + calibration
            //Do a copy here because we are going to change the activity flags of the SSP's
            FiffInfo infoTemp = *(m_pFiffInfo.data());

            //Turn on all SSP
            for(int i = 0; i < infoTemp.projs.size(); ++i) {
                infoTemp.projs[i].active = true;
            }

            //Create the projector for all SSP's on
            infoTemp.make_projector(matProjectors);
            //set columns of matrix to zero depending on bad channels indexes
            for(qint32 j = 0; j < infoTemp.bads.size(); ++j) {
                matProjectors.col(infoTemp.ch_names.indexOf(infoTemp.bads.at(j))).setZero();
            }
        }

        if(m_bUseComp) {
            // Setup Comps
            FiffCtfComp newComp;
            //Do this always from 0 since we always read new raw data, we never actually perform a multiplication on already existing data
            if(m_pFiffInfo->make_compensator(0, 101, newComp)) {
                matComp = newComp.data->data;
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

void Hpi::onDigitizersChanged(const QList<FIFFLIB::FiffDigPoint>& lDigitzers,
                              const QString& sFilePath)
{    
    m_mutex.lock();
    if(m_pFiffInfo) {
        m_pFiffInfo->dig = lDigitzers;
    }

    m_sFilePathDigitzers = sFilePath;

    m_mutex.unlock();
}

//=============================================================================================================

void Hpi::onDoSingleHpiFit()
{
    if(m_vCoilFreqs.size() < 3) {
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
    if(m_vCoilFreqs.size() < 3) {
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
    m_vCoilFreqs = vCoilFreqs;
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
//    if(m_vCoilFreqs.size() < 3) {
//       QMessageBox msgBox;
//       msgBox.setText("Please load a digitizer set with at least 3 HPI coils first.");
//       msgBox.exec();
//       return;
//    }

    m_bDoContinousHpi = bChecked;
}

//=============================================================================================================

void Hpi::setFittingWindowSize(int winSize)
{
    QMutexLocker locker(&m_mutex);
    m_iFittingWindowSize = winSize;
}

//=============================================================================================================

void Hpi::onDevHeadTransAvailable(const FIFFLIB::FiffCoordTrans& devHeadTrans)
{
    m_pFiffInfo->dev_head_t = devHeadTrans;
}

//=============================================================================================================

void Hpi::run()
{
    // Wait for fiff info
    bool bFiffInfo = false;

    while(true) {
        m_mutex.lock();
        if(m_pFiffInfo) {
            bFiffInfo = true;
        }
        m_mutex.unlock();
        if(bFiffInfo) {
            break;
        }
        msleep(100);
    }

    // init hpi fit
    HpiModelParameters hpiModelParameters = setModelParameters(m_vCoilFreqs,
                                                         m_pFiffInfo->sfreq,
                                                         m_pFiffInfo->linefreq,
                                                         false);
    HpiFitResult fitResult;
    fitResult.hpiFreqs = m_vCoilFreqs;
    fitResult.errorDistances = QVector<double>(m_vCoilFreqs.size());
    fitResult.devHeadTrans = m_pFiffInfo->dev_head_t;
    fitResult.devHeadTrans.from = 1;
    fitResult.devHeadTrans.to = 4;

    FiffCoordTrans transDevHeadRef = m_pFiffInfo->dev_head_t;

    HpiDataUpdater hpiDataUpdater = HpiDataUpdater(m_pFiffInfo);
    HPIFit HPI = HPIFit(hpiDataUpdater.getSensors());

    bool bDrop = false;
    double dErrorMax = 0.0;
    double dMeanErrorDist = 0.0;
    double dAllowedMovement = 0.0;
    double dAllowedRotation = 0.0;
    double dMovement = 0.0;
    double dRotation = 0.0;

    int iDataIndexCounter = 0;
    MatrixXd matData;

    m_mutex.lock();
    int fittingWindowSize = m_iFittingWindowSize;
    m_mutex.unlock();

    MatrixXd matDataMerged(m_pFiffInfo->chs.size(), fittingWindowSize);
    bool bOrder = false;

    while(!isInterruptionRequested()) {
        m_mutex.lock();
        if(fittingWindowSize != m_iFittingWindowSize) {
            fittingWindowSize = m_iFittingWindowSize;
            std::cout << "Fitting window size: " << fittingWindowSize << "\n";
            matDataMerged.resize(m_pFiffInfo->chs.size(), fittingWindowSize);
            iDataIndexCounter = 0;
        }
        m_mutex.unlock();

        //pop matrix
        if(m_pCircularBuffer->pop(matData)) {
            if(iDataIndexCounter + matData.cols() < matDataMerged.cols()) {
                matDataMerged.block(0, iDataIndexCounter, matData.rows(), matData.cols()) = matData;
                iDataIndexCounter += matData.cols();
            } else {
                m_mutex.lock();
                if(m_bDoSingleHpi) {
                    m_bDoSingleHpi = false;
                    fitResult = HpiFitResult();
                }
                fitResult.sFilePathDigitzers = m_sFilePathDigitzers;
                m_mutex.unlock();

                matDataMerged.block(0, iDataIndexCounter, matData.rows(), matDataMerged.cols()-iDataIndexCounter) =
                        matData.block(0, 0, matData.rows(), matDataMerged.cols()-iDataIndexCounter);

                m_mutex.lock();
                hpiModelParameters = setModelParameters(m_vCoilFreqs,
                                                     m_pFiffInfo->sfreq,
                                                     m_pFiffInfo->linefreq,
                                                     false);
                hpiDataUpdater.checkForUpdate(m_pFiffInfo);
                hpiDataUpdater.prepareDataAndProjectors(matDataMerged,m_matCompProjectors);
                bOrder = m_bDoFreqOrder;
                m_bDoFreqOrder = false;
                m_mutex.unlock();

                // Perform actual fitting
                HPI.fit(hpiDataUpdater.getProjectedData(),
                        hpiDataUpdater.getProjectors(),
                        hpiModelParameters,
                        hpiDataUpdater.getHpiDigitizer(),
                        bOrder,
                        fitResult);

                if(bOrder) {
                    m_mutex.lock();
                    m_vCoilFreqs = fitResult.hpiFreqs;
                    m_mutex.unlock();
                }

                //Check if the error meets distance requirement
                if(fitResult.errorDistances.size() > 0) {
                    dMeanErrorDist = std::accumulate(fitResult.errorDistances.begin(), fitResult.errorDistances.end(), .0) / fitResult.errorDistances.size();

                    emit errorsChanged(fitResult.errorDistances, dMeanErrorDist);

                    m_mutex.lock();
                    dErrorMax = m_dAllowedMeanErrorDist;
                    m_mutex.unlock();
                    if(dMeanErrorDist < dErrorMax) {
                        //If fit was good, set newly calculated transformation matrix to fiff info
                        emit devHeadTransAvailable(fitResult.devHeadTrans);

                        // check for large head movement
                        dMovement = transDevHeadRef.translationTo(fitResult.devHeadTrans.trans);
                        dRotation = transDevHeadRef.angleTo(fitResult.devHeadTrans.trans);

                        emit movementResultsChanged(dMovement,dRotation);

                        fitResult.fHeadMovementDistance = dMovement;
                        fitResult.fHeadMovementAngle = dRotation;
                        fitResult.bIsLargeHeadMovement = false;

                        m_mutex.lock();
                        dAllowedMovement = m_dAllowedMovement;
                        dAllowedRotation = m_dAllowedRotation;
                        m_mutex.unlock();
                        if(dMovement > dAllowedMovement || dRotation > dAllowedRotation) {
                            fitResult.bIsLargeHeadMovement = true;
                            transDevHeadRef = fitResult.devHeadTrans;
                        }

                        m_pHpiOutput->measurementData()->setValue(fitResult);
                    }
                }

                iDataIndexCounter = 0;
            }
        }
    }
}

//=============================================================================================================

QString Hpi::getBuildInfo()
{
    return QString(HPIPLUGIN::buildDateTime()) + QString(" - ")  + QString(HPIPLUGIN::buildHash());
}
