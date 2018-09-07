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

#include <disp/viewers/connectivitysettingsview.h>

#include <scMeas/realtimesourceestimate.h>
#include <scMeas/realtimeconnectivityestimate.h>
#include <scMeas/realtimemultisamplearray.h>
#include <scMeas/realtimeevokedset.h>

#include <mne/mne_epoch_data_list.h>
#include <mne/mne_bem.h>

#include <disp/viewers/connectivitysettingsview.h>

#include <realtime/rtProcessing/rtconnectivity.h>


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
using namespace REALTIMELIB;
using namespace IOBUFFER;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NeuronalConnectivity::NeuronalConnectivity()
: m_bIsRunning(false)
, m_iDownSample(3)
, m_iNumberAverages(10)
, m_sAtlasDir("./MNE-sample-data/subjects/sample/label")
, m_sSurfaceDir("./MNE-sample-data/subjects/sample/surf")
, m_sAvrType("4")
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
            &NeuronalConnectivity::updateRTE, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTEVSInput);

    // Output
    m_pRTCEOutput = PluginOutputData<RealTimeConnectivityEstimate>::create(this, "NeuronalConnectivityOut", "NeuronalConnectivity output data");
    m_outputConnectors.append(m_pRTCEOutput);
    m_pRTCEOutput->data()->setName(this->getName());

    //Add control widgets to output data (will be used by QuickControlView in RealTimeConnectivityEstimateWidget)
    ConnectivitySettingsView* pConnectivitySettingsView = new ConnectivitySettingsView();
    connect(pConnectivitySettingsView, &ConnectivitySettingsView::connectivityMetricChanged,
            this, &NeuronalConnectivity::onMetricChanged);
    m_pRTCEOutput->data()->addControlWidget(pConnectivitySettingsView);

    //Init rt connectivity worker
    m_pRtConnectivity = RtConnectivity::SPtr::create();
    connect(m_pRtConnectivity.data(), &RtConnectivity::newConnectivityResultAvailable,
            this, &NeuronalConnectivity::onNewConnectivityResultAvailable);

    m_pCircularNetworkBuffer = QSharedPointer<CircularBuffer<CONNECTIVITYLIB::Network> >(new CircularBuffer<CONNECTIVITYLIB::Network>(10));

    //Init connectivity settings
    m_connectivitySettings.m_sConnectivityMethods = QStringList() << "COR";
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
    return "Neuronal Connectivity";
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

        QList<MatrixXd> epochDataList;

        for(qint32 i = 0; i < pRTSE->getValue().size(); ++i) {
             epochDataList.append(pRTSE->getValue()[i]->data);
        }

        m_connectivitySettings.m_matDataList << epochDataList;

        m_timer.restart();
        m_pRtConnectivity->append(m_connectivitySettings);

        if(m_connectivitySettings.m_matDataList.size() >= m_iNumberAverages) {
            m_connectivitySettings.m_matDataList.removeFirst();
        }
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
            bool bPick = false;
            qint32 unit;
            int counter = 0;
            QString sChType = "mag";

            for(int i = 0; i < m_pFiffInfo->chs.size(); ++i) {
                unit = m_pFiffInfo->chs.at(i).unit;

                if(unit == FIFF_UNIT_T_M &&
                    sChType == "grad") {
                    bPick = true;
                } else if(unit == FIFF_UNIT_T &&
                            sChType == "mag") {
                    bPick = true;
                } else if (unit == FIFF_UNIT_V &&
                            sChType == "eeg") {
                    bPick = true;
                }

                if(bPick) {
                    //Get the positions
                    m_matNodeVertComb.conservativeResize(m_matNodeVertComb.rows()+1, 3);
                    m_matNodeVertComb(counter,0) = m_pFiffInfo->chs.at(i).chpos.r0(0);
                    m_matNodeVertComb(counter,1) = m_pFiffInfo->chs.at(i).chpos.r0(1);
                    m_matNodeVertComb(counter,2) = m_pFiffInfo->chs.at(i).chpos.r0(2);

                    m_chIdx << i;
                    counter++;
                }

                bPick = false;
            }

            //Set node 3D positions to connectivity settings
            m_connectivitySettings.m_matNodePositions = m_matNodeVertComb;
        }

        MatrixXd data;
        QList<MatrixXd> epochDataList;

        for(qint32 i = 0; i < pRTMSA->getMultiSampleArray().size(); ++i)
        {
            const MatrixXd& t_mat = pRTMSA->getMultiSampleArray()[i];
            data.resize(m_chIdx.size(), t_mat.cols());

            for(qint32 j = 0; j < m_chIdx.size(); ++j)
            {
                data.row(j) = t_mat.row(m_chIdx.at(j));
            }

            epochDataList.append(data);
        }

        m_connectivitySettings.m_matDataList << epochDataList;

        m_timer.restart();
        m_pRtConnectivity->append(m_connectivitySettings);

        if(m_connectivitySettings.m_matDataList.size() >= m_iNumberAverages) {
            m_connectivitySettings.m_matDataList.removeFirst();
        }
    }
}


//*************************************************************************************************************

void NeuronalConnectivity::updateRTE(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    QSharedPointer<RealTimeEvokedSet> pRTES = pMeasurement.dynamicCast<RealTimeEvokedSet>();

    if(pRTES) {
        //Fiff Information of the evoked
        if(!m_pFiffInfo && pRTES->getValue()->evoked.size() > 0) {
            for(int i = 0; i < pRTES->getValue()->evoked.size(); ++i) {
                if(pRTES->getValue()->evoked.at(i).comment == m_sAvrType) {
                    m_pFiffInfo = QSharedPointer<FiffInfo>(new FiffInfo(pRTES->getValue()->evoked.at(i).info));

                    //Set 3D sensor surface for visualization
                    QFile t_filesensorSurfaceVV(QCoreApplication::applicationDirPath() + "/resources/general/sensorSurfaces/306m_rt.fif");
                    MNEBem::SPtr pSensorSurfaceVV = MNEBem::SPtr::create(t_filesensorSurfaceVV);
                    m_pRTCEOutput->data()->setSensorSurface(pSensorSurfaceVV);
                    m_pRTCEOutput->data()->setFiffInfo(m_pFiffInfo);

                    //Generate node vertices
                    bool bPick = false;
                    qint32 unit;
                    int counter = 0;
                    QString sChType = "mag";

                    for(int i = 0; i < m_pFiffInfo->chs.size(); ++i) {
                        unit = m_pFiffInfo->chs.at(i).unit;

                        if(unit == FIFF_UNIT_T_M &&
                            sChType == "grad") {
                            bPick = true;
                        } else if(unit == FIFF_UNIT_T &&
                                    sChType == "mag") {
                            bPick = true;
                        } else if (unit == FIFF_UNIT_V &&
                                    sChType == "eeg") {
                            bPick = true;
                        }

                        if(bPick) {
                            //Get the positions
                            m_matNodeVertComb.conservativeResize(m_matNodeVertComb.rows()+1, 3);
                            m_matNodeVertComb(counter,0) = m_pFiffInfo->chs.at(i).chpos.r0(0);
                            m_matNodeVertComb(counter,1) = m_pFiffInfo->chs.at(i).chpos.r0(1);
                            m_matNodeVertComb(counter,2) = m_pFiffInfo->chs.at(i).chpos.r0(2);

                            m_chIdx << i;
                            counter++;
                        }

                        bPick = false;
                    }

                    //Set node 3D positions to connectivity settings
                    m_connectivitySettings.m_matNodePositions = m_matNodeVertComb;
                }
            }
        }

        FiffEvokedSet::SPtr pFiffEvokedSet = pRTES->getValue();

        for(int i = 0; i < pFiffEvokedSet->evoked.size(); ++i) {
            //qDebug()<<""<<m_sAvrType;
            if(pFiffEvokedSet->evoked.at(i).comment == m_sAvrType) {
                MatrixXd data;
                QList<MatrixXd> epochDataList;

                const MatrixXd& t_mat = pFiffEvokedSet->evoked.at(i).data;
                data.resize(m_chIdx.size(), t_mat.cols());

                for(qint32 j = 0; j < m_chIdx.size(); ++j)
                {
                    data.row(j) = t_mat.row(m_chIdx.at(j));
                }

                epochDataList.append(data);

                m_connectivitySettings.m_matDataList << epochDataList;

                m_timer.restart();
                m_pRtConnectivity->append(m_connectivitySettings);

                if(m_connectivitySettings.m_matDataList.size() >= m_iNumberAverages) {
                    m_connectivitySettings.m_matDataList.removeFirst();
                }
            }
        }
    }
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

    int skip_count = 0;

    while(m_bIsRunning)
    {
        //Do processing after skip count has reached limit
        if((skip_count % m_iDownSample) == 0)
        {
            //QMutexLocker locker(&m_mutex);
            //Do connectivity estimation here
            Network connectivityResult = m_pCircularNetworkBuffer->pop();

            //Send the data to the connected plugins and the online display
            if(!connectivityResult.isEmpty()) {
                //qDebug()<<"NeuronalConnectivity::run - Total time"<<m_timer.elapsed();
                m_pRTCEOutput->data()->setValue(connectivityResult);
            }
        }

        ++skip_count;
    }
}


//*************************************************************************************************************

void NeuronalConnectivity::onNewConnectivityResultAvailable(const Network& connectivityResult)
{
    //QMutexLocker locker(&m_mutex);
    m_pCircularNetworkBuffer->push(connectivityResult);
}


//*************************************************************************************************************

void NeuronalConnectivity::onMetricChanged(const QString& sMetric)
{
    m_connectivitySettings.m_sConnectivityMethods = QStringList() << sMetric;
}


//*************************************************************************************************************

void NeuronalConnectivity::onWindowTypeChanged(const QString& windowType)
{
    m_connectivitySettings.m_sWindowType = windowType;
}
