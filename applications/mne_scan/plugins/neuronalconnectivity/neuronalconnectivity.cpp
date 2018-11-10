//=============================================================================================================
/**
* @file     neuronalconnectivity.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     October, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Lorenz Esch and Matti Hamalainen. All rights reserved.
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

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "neuronalconnectivity.h"
#include "FormFiles/neuronalconnectivitysetupwidget.h"

#include <connectivity/connectivity.h>
#include <rtprocessing/rtconnectivity.h>

#include <disp/viewers/connectivitysettingsview.h>

#include <scMeas/realtimesourceestimate.h>
#include <scMeas/realtimeconnectivityestimate.h>
#include <scMeas/realtimemultisamplearray.h>
#include <scMeas/realtimeevokedset.h>

#include <mne/mne_epoch_data_list.h>
#include <mne/mne_bem.h>

#include <disp/viewers/connectivitysettingsview.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
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


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NeuronalConnectivity::NeuronalConnectivity()
: m_bIsRunning(false)
, m_iDownSample(1)
, m_iNumberAverages(10)
, m_sAvrType("4")
, m_iFreqBandLow(1)
, m_iFreqBandHigh(50)
, m_iBlockSize(1)
, m_pConnectivitySettingsView(ConnectivitySettingsView::SPtr::create())
{
}


//*************************************************************************************************************

NeuronalConnectivity::~NeuronalConnectivity()
{
    if(this->isRunning()) {
        stop();
    }
}


//*************************************************************************************************************

QSharedPointer<IPlugin> NeuronalConnectivity::clone() const
{
    QSharedPointer<NeuronalConnectivity> pNeuronalConnectivityClone(new NeuronalConnectivity);
    return pNeuronalConnectivityClone;
}


//*************************************************************************************************************

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

    m_pRTCEOutput->data()->addControlWidget(m_pConnectivitySettingsView);

    //Init rt connectivity worker
    m_pRtConnectivity = RtConnectivity::SPtr::create();
    connect(m_pRtConnectivity.data(), &RtConnectivity::newConnectivityResultAvailable,
            this, &NeuronalConnectivity::onNewConnectivityResultAvailable);

    m_pCircularNetworkBuffer = QSharedPointer<CircularBuffer<CONNECTIVITYLIB::Network> >(new CircularBuffer<CONNECTIVITYLIB::Network>(10));

    //Init connectivity settings
    m_sConnectivityMethods = QStringList() << "COR";
    m_connectivitySettings.m_sConnectivityMethods = m_sConnectivityMethods;
    m_connectivitySettings.m_sWindowType = "Hanning";
}


//*************************************************************************************************************

void NeuronalConnectivity::unload()
{

}


//*************************************************************************************************************

bool NeuronalConnectivity::start()
{
    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
    if(this->isRunning()) {
        QThread::wait();
    }

    m_bIsRunning = true;

    //Start thread
    QThread::start();

    return true;
}


//*************************************************************************************************************

bool NeuronalConnectivity::stop()
{
    m_bIsRunning = false;

    return true;
}


//*************************************************************************************************************

IPlugin::PluginType NeuronalConnectivity::getType() const
{
    return _IAlgorithm;
}


//*************************************************************************************************************

QString NeuronalConnectivity::getName() const
{
    return "Connectivity";
}


//*************************************************************************************************************

QWidget* NeuronalConnectivity::setupWidget()
{
    NeuronalConnectivitySetupWidget* setupWidget = new NeuronalConnectivitySetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}


//*************************************************************************************************************

void NeuronalConnectivity::updateSource(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    QSharedPointer<RealTimeSourceEstimate> pRTSE = pMeasurement.dynamicCast<RealTimeSourceEstimate>();

    if(pRTSE) {
        //Fiff information
        if(!m_pFiffInfo) {
            m_pFiffInfo = pRTSE->getFiffInfo();

            //Init output - Unocmment this if you also uncommented the m_pRTCEOutput in the constructor above
            m_pRTCEOutput->data()->setAnnotSet(pRTSE->getAnnotSet());
            m_pRTCEOutput->data()->setSurfSet(pRTSE->getSurfSet());
            m_pRTCEOutput->data()->setFwdSolution(pRTSE->getFwdSolution());
            m_pRTCEOutput->data()->setFiffInfo(m_pFiffInfo);

            //Generate node vertices
            if(pRTSE->getFwdSolution()->isClustered()) {
                m_matNodeVertLeft.resize(pRTSE->getFwdSolution()->src[0].cluster_info.centroidVertno.size(),3);

                for(int j = 0; j < m_matNodeVertLeft.rows(); ++j) {
                    m_matNodeVertLeft.row(j) = pRTSE->getSurfSet()->data()[0].rr().row(pRTSE->getFwdSolution()->src[0].cluster_info.centroidVertno.at(j)) - pRTSE->getSurfSet()->data()[0].offset().transpose();
                }

                m_matNodeVertRight.resize(pRTSE->getFwdSolution()->src[1].cluster_info.centroidVertno.size(),3);
                for(int j = 0; j < m_matNodeVertRight.rows(); ++j) {
                    m_matNodeVertRight.row(j) = pRTSE->getSurfSet()->data()[1].rr().row(pRTSE->getFwdSolution()->src[1].cluster_info.centroidVertno.at(j)) - pRTSE->getSurfSet()->data()[1].offset().transpose();
                }
            } else {
                m_matNodeVertLeft.resize(pRTSE->getFwdSolution()->src[0].vertno.rows(),3);
                for(int j = 0; j < m_matNodeVertLeft.rows(); ++j) {
                    m_matNodeVertLeft.row(j) = pRTSE->getSurfSet()->data()[0].rr().row(pRTSE->getFwdSolution()->src[0].vertno(j)) - pRTSE->getSurfSet()->data()[0].offset().transpose();
                }

                m_matNodeVertRight.resize(pRTSE->getFwdSolution()->src[1].vertno.rows(),3);
                for(int j = 0; j < m_matNodeVertRight.rows(); ++j) {
                    m_matNodeVertRight.row(j) = pRTSE->getSurfSet()->data()[1].rr().row(pRTSE->getFwdSolution()->src[1].vertno(j)) - pRTSE->getSurfSet()->data()[1].offset().transpose();
                }
            }

            m_matNodeVertComb.resize(m_matNodeVertLeft.rows() + m_matNodeVertRight.rows(),3);
            m_matNodeVertComb << m_matNodeVertLeft, m_matNodeVertRight;

            //Set node 3D positions to connectivity settings
            m_connectivitySettings.m_matNodePositions = m_matNodeVertComb;            
        }

        for(qint32 i = 0; i < pRTSE->getValue().size(); ++i) {
            m_iBlockSize = pRTSE->getValue().first()->data.cols();

            // Check row and colum integrity and restart if necessary
            if(m_connectivitySettings.size() != 0) {
                if(m_iBlockSize != m_connectivitySettings.m_dataList.first().matData.cols()) {
                    m_connectivitySettings.clearData();
                    m_pRtConnectivity->restart();
                }
            }

            m_connectivitySettings.append(pRTSE->getValue()[i]->data);
        }

        //Pop data from buffer
        if(m_connectivitySettings.size() > m_iNumberAverages) {
            m_pRtConnectivity->restart();
            int size = m_connectivitySettings.size();

            for(int i = 0; i < size-m_iNumberAverages; ++i) {
                m_connectivitySettings.removeFirst();
            }
        }

        m_timer.restart();
        m_pRtConnectivity->append(m_connectivitySettings);
    }
}


//*************************************************************************************************************

void NeuronalConnectivity::updateRTMSA(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<RealTimeMultiSampleArray>();

    if(pRTMSA) {
        //Fiff information
        if(!m_pFiffInfo) {
            m_pFiffInfo = pRTMSA->info();

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
            QList<MatrixXd> epochDataList;

            for(qint32 i = 0; i < pRTMSA->getMultiSampleArray().size(); ++i) {
                const MatrixXd& t_mat = pRTMSA->getMultiSampleArray()[i];
                m_iBlockSize = pRTMSA->getMultiSampleArray()[i].cols();

                // Check row and colum integrity and restart if necessary
                if(m_connectivitySettings.size() != 0) {
                    if(m_iBlockSize != m_connectivitySettings.m_dataList.first().matData.cols()) {
                        m_connectivitySettings.clearData();
                        m_pRtConnectivity->restart();
                    }
                }

                data.resize(m_chIdx.size(), t_mat.cols());

                for(qint32 j = 0; j < m_chIdx.size(); ++j) {
                    data.row(j) = t_mat.row(m_chIdx.at(j));
                }

                m_connectivitySettings.append(data);
            }

            //Pop data from buffer
            if(m_connectivitySettings.size() > m_iNumberAverages) {
                m_pRtConnectivity->restart();
                int size = m_connectivitySettings.size();

                for(int i = 0; i < size-m_iNumberAverages; ++i) {
                    m_connectivitySettings.removeFirst();
                }
            }

            m_timer.restart();
            m_pRtConnectivity->append(m_connectivitySettings);
        }
    }
}


//*************************************************************************************************************

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
                        if(m_iBlockSize != m_connectivitySettings.m_dataList.first().matData.cols()) {
                            m_connectivitySettings.clearData();
                            m_pRtConnectivity->restart();
                        }
                    }

                    MatrixXd data;
                    data.resize(m_chIdx.size(), t_mat.cols());

                    for(qint32 j = 0; j < m_chIdx.size(); ++j) {
                        data.row(j) = t_mat.row(m_chIdx.at(j));
                    }

                    m_connectivitySettings.append(data);

                    //Pop data from buffer
                    if(m_connectivitySettings.size() > m_iNumberAverages) {
                        m_pRtConnectivity->restart();
                        int size = m_connectivitySettings.size();

                        for(int i = 0; i < size-m_iNumberAverages; ++i) {
                            m_connectivitySettings.removeFirst();
                        }
                    }

                    m_timer.restart();
                    m_pRtConnectivity->append(m_connectivitySettings);

                    break;
                }
            }
        }
    }
}


//*************************************************************************************************************

void NeuronalConnectivity::generateNodeVertices()
{
    //Generate node vertices
    bool bPick = false;
    qint32 unit, kind;
    int counter = 0;
    QString sChType = "grad";
    m_chIdx.clear();
    m_matNodeVertComb = MatrixX3f();

    for(int i = 0; i < m_pFiffInfo->chs.size(); ++i) {
        unit = m_pFiffInfo->chs.at(i).unit;
        kind = m_pFiffInfo->chs.at(i).kind;

        if(unit == FIFF_UNIT_T_M &&
            kind == FIFFV_MEG_CH &&
            sChType == "grad") {
            bPick = true;

            //Skip second gradiometer in triplet
            ++i;
        } else if(unit == FIFF_UNIT_T &&
                  kind == FIFFV_MEG_CH &&
                    sChType == "mag") {
            bPick = true;
        } else if (unit == FIFF_UNIT_V &&
                   kind == FIFFV_EEG_CH &&
                    sChType == "eeg") {
            bPick = true;
        }

        if(bPick && !m_pFiffInfo->bads.contains(m_pFiffInfo->chs.at(i).ch_name)) {
            //Get the positions
            m_matNodeVertComb.conservativeResize(m_matNodeVertComb.rows()+1, 3);
            m_matNodeVertComb(counter,0) = m_pFiffInfo->chs.at(i).chpos.r0(0);
            m_matNodeVertComb(counter,1) = m_pFiffInfo->chs.at(i).chpos.r0(1);
            m_matNodeVertComb(counter,2) = m_pFiffInfo->chs.at(i).chpos.r0(2);

            if(sChType == "grad") {
                m_chIdx << i-1;
            } else {
                m_chIdx << i;
            }

            counter++;
        }

        bPick = false;
    }

    //Set node 3D positions to connectivity settings
    m_connectivitySettings.m_matNodePositions = m_matNodeVertComb;
    m_connectivitySettings.clearData();
}


//*************************************************************************************************************

void NeuronalConnectivity::run()
{
    //
    // Wait for Fiff Info
    //
    while(!m_pFiffInfo) {
        msleep(10);
    }    

    // Init the frequency band selection to 1 to 50Hz
    onFrequencyBandChanged(1,50);

    int skip_count = 0;

    while(m_bIsRunning)
    {
        //Do processing after skip count has reached limit
        if((skip_count % m_iDownSample) == 0)
        {
            //QMutexLocker locker(&m_mutex);
            //Do connectivity estimation here
            m_currentConnectivityResult = m_pCircularNetworkBuffer->pop();

            //Send the data to the connected plugins and the online display
            if(!m_currentConnectivityResult.isEmpty()) {
                //qDebug()<<"NeuronalConnectivity::run - Total time"<<m_timer.elapsed();
                m_currentConnectivityResult.setFrequencyBins(m_iFreqBandLow, m_iFreqBandHigh);
                m_currentConnectivityResult.normalize();
                m_pRTCEOutput->data()->setValue(m_currentConnectivityResult);
            } else {
                qDebug()<<"NeuronalConnectivity::run - Network is empty";
            }
        }

        ++skip_count;
    }
}


//*************************************************************************************************************

void NeuronalConnectivity::onNewConnectivityResultAvailable(const Network& connectivityResult,
                                                            const ConnectivitySettings& connectivitySettings)
{
    //QMutexLocker locker(&m_mutex);
    m_connectivitySettings = connectivitySettings;
    m_connectivitySettings.m_sConnectivityMethods = m_sConnectivityMethods;
    m_pCircularNetworkBuffer->push(connectivityResult);
}


//*************************************************************************************************************

void NeuronalConnectivity::onMetricChanged(const QString& sMetric)
{
    m_pRtConnectivity->restart();
    m_sConnectivityMethods = QStringList() << sMetric;
}


//*************************************************************************************************************

void NeuronalConnectivity::onNumberTrialsChanged(int iNumberTrials)
{
    m_iNumberAverages = iNumberTrials;
}


//*************************************************************************************************************

void NeuronalConnectivity::onWindowTypeChanged(const QString& windowType)
{
    if(m_connectivitySettings.m_sWindowType != windowType) {
        m_connectivitySettings.resetData();
        m_connectivitySettings.m_sWindowType = windowType;
    }
}


//*************************************************************************************************************

void NeuronalConnectivity::onTriggerTypeChanged(const QString& triggerType)
{
    if(triggerType != m_sAvrType) {
        m_connectivitySettings.clearData();
        m_sAvrType = triggerType;
    }
}


//*************************************************************************************************************

void NeuronalConnectivity::onFrequencyBandChanged(int iFreqLow, int iFreqHigh)
{
    // By default the number of frequency bins is half the signal since we only use the half spectrum
    double dScaleFactor = m_iBlockSize/m_pFiffInfo->sfreq;

    // Convert to frequency bins
    m_iFreqBandLow = iFreqLow * dScaleFactor;
    m_iFreqBandHigh = iFreqHigh * dScaleFactor;

    //QMutexLocker locker(&m_mutex);
    if(!m_currentConnectivityResult.isEmpty()) {
        m_currentConnectivityResult.setFrequencyBins(m_iFreqBandLow, m_iFreqBandHigh);
        m_currentConnectivityResult.normalize();
        m_pCircularNetworkBuffer->push(m_currentConnectivityResult);
    }

    //qDebug() << "NeuronalConnectivity::onFrequencyBandChanged - m_iFreqBandLow" << m_iFreqBandLow;
    //qDebug() << "NeuronalConnectivity::onFrequencyBandChanged - m_iFreqBandHigh" << m_iFreqBandHigh;
}
