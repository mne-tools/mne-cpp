//=============================================================================================================
/**
 * @file     neuronalconnectivity.cpp
 * @author   Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     October, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Gabriel B Motta, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the NeuronalConnectivity class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "neuronalconnectivity.h"
#include "FormFiles/neuronalconnectivitysetupwidget.h"

#include <connectivity/connectivity.h>
#include <connectivity/metrics/abstractmetric.h>
#include <rtprocessing/rtconnectivity.h>

#include <disp/viewers/connectivitysettingsview.h>

#include <scMeas/realtimesourceestimate.h>
#include <scMeas/realtimeconnectivityestimate.h>
#include <scMeas/realtimemultisamplearray.h>
#include <scMeas/realtimeevokedset.h>

#include <mne/mne_epoch_data_list.h>
#include <mne/mne_bem.h>

#include <disp/viewers/connectivitysettingsview.h>


//=============================================================================================================
// QT INCLUDES
//=============================================================================================================


//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================


//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace NEURONALCONNECTIVITYPLUGIN;
using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace MNELIB;
using namespace CONNECTIVITYLIB;
using namespace DISPLIB;
using namespace IOBUFFER;
using namespace RTPROCESSINGLIB;
using namespace Eigen;
using namespace FSLIB;
using namespace FIFFLIB;


//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NeuronalConnectivity::NeuronalConnectivity()
: m_bIsRunning(false)
, m_iDownSample(1)
, m_iNumberAverages(10)
, m_sAvrType("1")
, m_fFreqBandLow(7.0f)
, m_fFreqBandHigh(13.0f)
, m_iBlockSize(1)
, m_pConnectivitySettingsView(ConnectivitySettingsView::SPtr::create(this->getName()))
, m_pActionShowYourWidget(Q_NULLPTR)
, m_iNumberBadChannels(0)
{
    AbstractMetric::m_bStorageModeIsActive = true;
    AbstractMetric::m_iNumberBinStart = 0;
    AbstractMetric::m_iNumberBinAmount = 100;
}


//=============================================================================================================

NeuronalConnectivity::~NeuronalConnectivity()
{
    if(this->isRunning()) {
        stop();
    }
}


//=============================================================================================================

QSharedPointer<IPlugin> NeuronalConnectivity::clone() const
{
    QSharedPointer<NeuronalConnectivity> pNeuronalConnectivityClone(new NeuronalConnectivity);
    return pNeuronalConnectivityClone;
}


//=============================================================================================================

void NeuronalConnectivity::init()
{
    // Input
    m_pRTSEInput = PluginInputData<RealTimeSourceEstimate>::create(this, "NeuronalConnectivityInSource", "NeuronalConnectivity source input data");
    connect(m_pRTSEInput.data(), &PluginInputConnector::notify,
            this, &NeuronalConnectivity::updateSource, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTSEInput);

    m_pRTMSAInput = PluginInputData<RealTimeMultiSampleArray>::create(this, "NeuronalConnectivityInSensor", "NeuronalConnectivity sensor input data");
    connect(m_pRTMSAInput.data(), &PluginInputConnector::notify,
            this, &NeuronalConnectivity::updateRTMSA, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTMSAInput);

    m_pRTEVSInput = PluginInputData<RealTimeEvokedSet>::create(this, "NeuronalConnectivityInSensorEvoked", "NeuronalConnectivity evoked input data");
    connect(m_pRTEVSInput.data(), &PluginInputConnector::notify, this,
            &NeuronalConnectivity::updateRTEV, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTEVSInput);

    // Output
    m_pRTCEOutput = PluginOutputData<RealTimeConnectivityEstimate>::create(this, "NeuronalConnectivityOut", "NeuronalConnectivity output data");
    m_outputConnectors.append(m_pRTCEOutput);
    m_pRTCEOutput->data()->setName(this->getName());

    //Add control widgets to output data (will be used by QuickControlView by the measurements display)
    connect(m_pConnectivitySettingsView.data(), &ConnectivitySettingsView::connectivityMetricChanged,
            this, &NeuronalConnectivity::onMetricChanged);
    connect(m_pConnectivitySettingsView.data(), &ConnectivitySettingsView::numberTrialsChanged,
            this, &NeuronalConnectivity::onNumberTrialsChanged);
    connect(m_pConnectivitySettingsView.data(), &ConnectivitySettingsView::triggerTypeChanged,
            this, &NeuronalConnectivity::onTriggerTypeChanged);
    connect(m_pConnectivitySettingsView.data(), &ConnectivitySettingsView::freqBandChanged,
            this, &NeuronalConnectivity::onFrequencyBandChanged);

    onFrequencyBandChanged(m_pConnectivitySettingsView->getLowerFreq(), m_pConnectivitySettingsView->getUpperFreq());
    onMetricChanged(m_pConnectivitySettingsView->getConnectivityMetric());
    onWindowTypeChanged(m_pConnectivitySettingsView->getWindowType());
    onNumberTrialsChanged(m_pConnectivitySettingsView->getNumberTrials());
    onTriggerTypeChanged(m_pConnectivitySettingsView->getTriggerType());

    m_pRTCEOutput->data()->addControlWidget(m_pConnectivitySettingsView);

    //Init rt connectivity worker
    m_pRtConnectivity = RtConnectivity::SPtr::create();
    connect(m_pRtConnectivity.data(), &RtConnectivity::newConnectivityResultAvailable,
            this, &NeuronalConnectivity::onNewConnectivityResultAvailable);

    m_pCircularNetworkBuffer = QSharedPointer<CircularBuffer<CONNECTIVITYLIB::Network> >(new CircularBuffer<CONNECTIVITYLIB::Network>(10));
}


//=============================================================================================================

void NeuronalConnectivity::unload()
{

}


//=============================================================================================================

bool NeuronalConnectivity::start()
{
//    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
//    if(this->isRunning()) {
//        QThread::wait();
//    }

    m_bIsRunning = true;

    //Start thread
    QThread::start();

    return true;
}


//=============================================================================================================

bool NeuronalConnectivity::stop()
{
    m_bIsRunning = false;

    return true;
}


//=============================================================================================================

IPlugin::PluginType NeuronalConnectivity::getType() const
{
    return _IAlgorithm;
}


//=============================================================================================================

QString NeuronalConnectivity::getName() const
{
    return "Connectivity";
}


//=============================================================================================================

QWidget* NeuronalConnectivity::setupWidget()
{
    NeuronalConnectivitySetupWidget* setupWidget = new NeuronalConnectivitySetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}


//=============================================================================================================

void NeuronalConnectivity::updateSource(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    QSharedPointer<RealTimeSourceEstimate> pRTSE = pMeasurement.dynamicCast<RealTimeSourceEstimate>();

    if(pRTSE) {
        //Fiff information
        if(!m_pFiffInfo) {
            m_pFiffInfo = pRTSE->getFiffInfo();

            m_connectivitySettings.setSamplingFrequency(m_pFiffInfo->sfreq);

            //Init output - Unocmment this if you also uncommented the m_pRTCEOutput in the constructor above
            m_pRTCEOutput->data()->setAnnotSet(pRTSE->getAnnotSet());
            m_pRTCEOutput->data()->setSurfSet(pRTSE->getSurfSet());
            m_pRTCEOutput->data()->setFwdSolution(pRTSE->getFwdSolution());
            m_pRTCEOutput->data()->setFiffInfo(m_pFiffInfo);

            // Generate network nodes
            m_connectivitySettings.setNodePositions(*pRTSE->getFwdSolution(), *pRTSE->getSurfSet());
        }

        for(qint32 i = 0; i < pRTSE->getValue().size(); ++i) {
            // Find out how many samples were used for pre stimulus
            int iZeroIdx = 0;
            for(int j = 0; j < pRTSE->getValue()[i]->times.cols(); ++j) {
                if(pRTSE->getValue()[i]->times(j) >= 0) {
                    iZeroIdx = j;
                    //iZeroIdx = j + m_pFiffInfo->sfreq * 0.01; //Cut stimulus artifact, e.g. for median nerve stimulation.
                    break;
                }
            }

            m_iBlockSize = pRTSE->getValue().first()->data.cols() - iZeroIdx;

            // Check row and colum integrity and restart if necessary
            if(m_connectivitySettings.size() != 0) {
                if(m_iBlockSize != m_connectivitySettings.at(0).matData.cols()) {
                    m_connectivitySettings.clearAllData();
                    m_pRtConnectivity->restart();
                }
            }

            m_connectivitySettings.append(pRTSE->getValue()[i]->data.block(0,
                                                                           iZeroIdx,
                                                                           pRTSE->getValue()[i]->data.rows(),
                                                                           pRTSE->getValue()[i]->data.cols() - iZeroIdx));
        }

        //Pop data from buffer
        if(m_connectivitySettings.size() >= m_iNumberAverages) {
            m_pRtConnectivity->restart();
            m_connectivitySettings.removeFirst(m_connectivitySettings.size()-m_iNumberAverages);
        }

        m_timer.restart();
        m_pRtConnectivity->append(m_connectivitySettings);
    }
}


//=============================================================================================================

void NeuronalConnectivity::updateRTMSA(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<RealTimeMultiSampleArray>();

    if(pRTMSA) {
        //Fiff information
        if(!m_pFiffInfo) {
            m_pFiffInfo = pRTMSA->info();

            m_connectivitySettings.setSamplingFrequency(m_pFiffInfo->sfreq);

            //Set 3D sensor surface for visualization
            QFile t_filesensorSurfaceVV(QCoreApplication::applicationDirPath() + "/resources/general/sensorSurfaces/306m_rt.fif");
            MNEBem::SPtr pSensorSurfaceVV = MNEBem::SPtr::create(t_filesensorSurfaceVV);
            m_pRTCEOutput->data()->setSensorSurface(pSensorSurfaceVV);
            m_pRTCEOutput->data()->setFiffInfo(m_pFiffInfo);

            //Generate node vertices
            generateNodeVertices();
            m_iNumberBadChannels = m_pFiffInfo->bads.size();
        }

        if(m_pFiffInfo) {
            //Generate node vertices because the number of bad channels changed
            if(m_iNumberBadChannels != pRTMSA->info()->bads.size()) {
                m_pFiffInfo = pRTMSA->info();
                generateNodeVertices();
                m_iNumberBadChannels = m_pFiffInfo->bads.size();
                m_pRtConnectivity->restart();
            }

            MatrixXd data;

            for(qint32 i = 0; i < pRTMSA->getMultiSampleArray().size(); ++i) {
                const MatrixXd& t_mat = pRTMSA->getMultiSampleArray()[i];
                m_iBlockSize = pRTMSA->getMultiSampleArray()[i].cols();

                // Check row and colum integrity and restart if necessary
                if(m_connectivitySettings.size() != 0) {
                    if(m_iBlockSize != m_connectivitySettings.at(0).matData.cols()) {
                        m_connectivitySettings.clearAllData();
                        m_pRtConnectivity->restart();
                    }
                }

                data.resize(m_vecPicks.cols(), t_mat.cols());

                for(qint32 j = 0; j < m_vecPicks.cols(); ++j) {
                    data.row(j) = t_mat.row(m_vecPicks[j]);
                }

                m_connectivitySettings.append(data);
            }

            //Pop data from buffer
            if(m_connectivitySettings.size() > m_iNumberAverages) {
                m_pRtConnectivity->restart();
                m_connectivitySettings.removeFirst(m_connectivitySettings.size()-m_iNumberAverages);
            }

            m_timer.restart();
            m_pRtConnectivity->append(m_connectivitySettings);
        }
    }
}


//=============================================================================================================

void NeuronalConnectivity::updateRTEV(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    QSharedPointer<RealTimeEvokedSet> pRTEV = pMeasurement.dynamicCast<RealTimeEvokedSet>();

    if(pRTEV) {
        FiffEvokedSet::SPtr pFiffEvokedSet = pRTEV->getValue();
        QStringList lResponsibleTriggerTypes = pRTEV->getResponsibleTriggerTypes();

        if(m_pConnectivitySettingsView) {
            m_pConnectivitySettingsView->setTriggerTypes(lResponsibleTriggerTypes);
        }

        if(!pFiffEvokedSet || !lResponsibleTriggerTypes.contains(m_sAvrType)) {
            return;
        }

        //qDebug() << "NeuronalConnectivity::updateRTEV - Found trigger" << m_sAvrType;

        //Fiff Information of the evoked
        if(!m_pFiffInfo && pFiffEvokedSet->evoked.size() > 0) {
            for(int i = 0; i < pFiffEvokedSet->evoked.size(); ++i) {
                if(pFiffEvokedSet->evoked.at(i).comment == m_sAvrType) {
                    m_pFiffInfo = QSharedPointer<FiffInfo>(new FiffInfo(pFiffEvokedSet->info));

                    m_connectivitySettings.setSamplingFrequency(m_pFiffInfo->sfreq);

                    //Set 3D sensor surface for visualization
                    QFile t_filesensorSurfaceVV(QCoreApplication::applicationDirPath() + "/resources/general/sensorSurfaces/306m_rt.fif");
                    MNEBem::SPtr pSensorSurfaceVV = MNEBem::SPtr::create(t_filesensorSurfaceVV);
                    m_pRTCEOutput->data()->setSensorSurface(pSensorSurfaceVV);
                    m_pRTCEOutput->data()->setFiffInfo(m_pFiffInfo);

                    //Generate node vertices
                    generateNodeVertices();
                    m_iNumberBadChannels = m_pFiffInfo->bads.size();

                    break;
                }
            }
        }

        if(m_pFiffInfo) {
            if(!pFiffEvokedSet->evoked.isEmpty()) {
                m_iBlockSize = pFiffEvokedSet->evoked.first().data.cols();
            }

            for(int i = 0; i < pFiffEvokedSet->evoked.size(); ++i) {
                if(pFiffEvokedSet->evoked.at(i).comment == m_sAvrType) {
                    const MatrixXd& t_mat = pFiffEvokedSet->evoked.at(i).data;

                    m_iBlockSize = t_mat.cols();

                    // Check row and colum integrity and restart if necessary
                    if(m_connectivitySettings.size() != 0) {
                        if(m_iBlockSize != m_connectivitySettings.at(0).matData.cols()) {
                            m_connectivitySettings.clearAllData();
                            m_pRtConnectivity->restart();
                        }
                    }

                    MatrixXd data;
                    data.resize(m_vecPicks.cols(), t_mat.cols());

                    for(qint32 j = 0; j < m_vecPicks.cols(); ++j) {
                        data.row(j) = t_mat.row(m_vecPicks[j]);
                    }

                    m_connectivitySettings.append(data);

                    //Pop data from buffer
                    if(m_connectivitySettings.size() > m_iNumberAverages) {
                        m_pRtConnectivity->restart();
                        m_connectivitySettings.removeFirst(m_connectivitySettings.size()-m_iNumberAverages);
                    }

                    m_timer.restart();
                    m_pRtConnectivity->append(m_connectivitySettings);

                    break;
                }
            }
        }
    }
}


//=============================================================================================================

void NeuronalConnectivity::generateNodeVertices()
{
    if(!m_pFiffInfo) {
        qDebug() << "NeuronalConnectivity::generateNodeVertices - FiffInfo is Null. Returning.";
        return;
    }

    QString sCoilType = "grad";
    QString sChType = "meg";

    QStringList exclude;
    exclude << m_pFiffInfo->bads << m_pFiffInfo->ch_names.filter("EOG");

    if(sChType.contains("EEG", Qt::CaseInsensitive)) {
        m_vecPicks = m_pFiffInfo->pick_types(false,true,false,QStringList(),exclude);
    } else if(sCoilType.contains("grad", Qt::CaseInsensitive)) {
        // Only pick every second gradiometer which are not marked as bad.
        RowVectorXi picksTmp = m_pFiffInfo->pick_types(QString("grad"),false,false);
        m_vecPicks.resize(0);

        for(int i = 0; i < picksTmp.cols()-1; i+=2) {
            if(!m_pFiffInfo->bads.contains(m_pFiffInfo->ch_names.at(picksTmp(i)))) {
                m_vecPicks.conservativeResize(m_vecPicks.cols()+1);
                m_vecPicks(m_vecPicks.cols()-1) = picksTmp(i);
            }
        }
    } else if (sCoilType.contains("mag", Qt::CaseInsensitive)) {
        m_vecPicks = m_pFiffInfo->pick_types(QString("mag"),false,false,QStringList(),exclude);
    }

    // Set sampling frequency so that the spectrum resolution is updated
    m_connectivitySettings.setSamplingFrequency(m_pFiffInfo->sfreq);

    //Set node 3D positions to connectivity settings
    m_connectivitySettings.setNodePositions(*m_pFiffInfo, m_vecPicks);
    m_connectivitySettings.clearAllData();
}


//=============================================================================================================

void NeuronalConnectivity::run()
{
    // Wait for Fiff Info
    while(!m_pFiffInfo) {
        msleep(10);
    }    

    int skip_count = 0;

    while(m_bIsRunning) {
        //Do processing after skip count has reached limit
        if((skip_count % m_iDownSample) == 0) {
            //QMutexLocker locker(&m_mutex);
            //Do connectivity estimation here
            m_currentConnectivityResult = m_pCircularNetworkBuffer->pop();

            //Send the data to the connected plugins and the online display
            if(!m_currentConnectivityResult.isEmpty()) {
                //qDebug()<<"NeuronalConnectivity::run - Total time"<<m_timer.elapsed();
                m_currentConnectivityResult.setFrequencyRange(m_fFreqBandLow, m_fFreqBandHigh);
                m_currentConnectivityResult.normalize();

                m_pRTCEOutput->data()->setValue(m_currentConnectivityResult);
            } else {
                qDebug()<<"NeuronalConnectivity::run - Network is empty";
            }

        }

        ++skip_count;
    }
}


//=============================================================================================================

void NeuronalConnectivity::onNewConnectivityResultAvailable(const QList<Network>& connectivityResults,
                                                            const ConnectivitySettings& connectivitySettings)
{
    //QMutexLocker locker(&m_mutex);
    m_connectivitySettings = connectivitySettings;
    m_connectivitySettings.setConnectivityMethods(m_sConnectivityMethods);

    for(int i = 0; i < connectivityResults.size(); ++i) {
        m_pCircularNetworkBuffer->push(connectivityResults.at(i));
    }
}


//=============================================================================================================

void NeuronalConnectivity::onMetricChanged(const QString& sMetric)
{
    if(m_sConnectivityMethods.contains(sMetric)) {
        return;
    }

    m_sConnectivityMethods = QStringList() << sMetric;
    m_connectivitySettings.setConnectivityMethods(m_sConnectivityMethods);
    if(m_pRtConnectivity && m_bIsRunning) {
        m_pRtConnectivity->restart();
        m_pRtConnectivity->append(m_connectivitySettings);
    }
}


//=============================================================================================================

void NeuronalConnectivity::onNumberTrialsChanged(int iNumberTrials)
{
    m_iNumberAverages = iNumberTrials;
}


//=============================================================================================================

void NeuronalConnectivity::onWindowTypeChanged(const QString& windowType)
{
    if(m_connectivitySettings.getWindowType() != windowType) {
        m_connectivitySettings.clearIntermediateData();
        m_connectivitySettings.setWindowType(windowType);
    }
}


//=============================================================================================================

void NeuronalConnectivity::onTriggerTypeChanged(const QString& triggerType)
{
    if(triggerType != m_sAvrType) {
        m_connectivitySettings.clearAllData();
        m_sAvrType = triggerType;
    }
}


//=============================================================================================================

void NeuronalConnectivity::onFrequencyBandChanged(float fFreqLow, float fFreqHigh)
{
    // Convert to frequency bins
    m_fFreqBandLow = fFreqLow;
    m_fFreqBandHigh = fFreqHigh;

    //QMutexLocker locker(&m_mutex);
    if(!m_currentConnectivityResult.isEmpty()) {
        m_currentConnectivityResult.setFrequencyRange(m_fFreqBandLow, m_fFreqBandHigh);
        //m_currentConnectivityResult.normalize();
        m_pCircularNetworkBuffer->push(m_currentConnectivityResult);
    }

    //qDebug() << "NeuronalConnectivity::onFrequencyBandChanged - m_fFreqBandLow" << m_fFreqBandLow;
    //qDebug() << "NeuronalConnectivity::onFrequencyBandChanged - m_fFreqBandHigh" << m_fFreqBandHigh;
}
