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

#include <connectivity/connectivity.h>
#include <connectivity/network/network.h>

#include <disp/viewers/connectivitysettingsview.h>

#include <scMeas/realtimesourceestimate.h>
#include <scMeas/realtimeconnectivityestimate.h>
#include <scMeas/realtimemultisamplearray.h>

#include "FormFiles/neuronalconnectivitysetupwidget.h"

#include <mne/mne_epoch_data_list.h>

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
using namespace IOBUFFER;
using namespace MNELIB;
using namespace CONNECTIVITYLIB;
using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NeuronalConnectivity::NeuronalConnectivity()
: m_bIsRunning(false)
, m_iDownSample(3)
, m_pRTSEInput(Q_NULLPTR)
, m_pRTCEOutput(Q_NULLPTR)
, m_pNeuronalConnectivityBuffer(CircularMatrixBuffer<double>::SPtr())
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
    connect(m_pRTSEInput.data(), &PluginInputConnector::notify, this, &NeuronalConnectivity::updateSource, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTSEInput);

    m_pRTMSAInput = PluginInputData<RealTimeMultiSampleArray>::create(this, "NeuronalConnectivityInSensor", "NeuronalConnectivity sensor input data");
    connect(m_pRTMSAInput.data(), &PluginInputConnector::notify, this, &NeuronalConnectivity::updateRTMSA, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTMSAInput);

    m_pRTCEOutput = PluginOutputData<RealTimeConnectivityEstimate>::create(this, "NeuronalConnectivityOut", "NeuronalConnectivity output data");
    m_outputConnectors.append(m_pRTCEOutput);
    m_pRTCEOutput->data()->setName(this->getName());

    //Add control widgets to output data (will be used by QuickControlView in RealTimeConnectivityEstimateWidget)
    ConnectivitySettingsView* pConnectivitySettingsView = new ConnectivitySettingsView();
    connect(pConnectivitySettingsView, &ConnectivitySettingsView::connectivityMetricChanged,
            this, &NeuronalConnectivity::onMetricChanged);
    m_pRTCEOutput->data()->addControlWidget(pConnectivitySettingsView);

    //Init connectivity settings
    m_connectivitySettings.m_sConnectivityMethods = QStringList() << "COR";
    m_connectivitySettings.m_sWindowType = "Hanning";

    //Delete Buffer - will be initialized with first incoming data
    if(!m_pNeuronalConnectivityBuffer.isNull()) {
        m_pNeuronalConnectivityBuffer = CircularMatrixBuffer<double>::SPtr();
    }
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

    m_pNeuronalConnectivityBuffer->releaseFromPop();
    m_pNeuronalConnectivityBuffer->releaseFromPush();

    m_pNeuronalConnectivityBuffer->clear();

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
        //Check if buffer initialized
        if(!m_pNeuronalConnectivityBuffer) {
            m_pNeuronalConnectivityBuffer = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(64, pRTSE->getValue()->data.rows(), pRTSE->getValue()->data.cols()));
        }

        //Fiff information
        if(!m_pFiffInfo) {
            m_pFiffInfo = pRTSE->getFiffInfo();

            //Init output - Unocmment this if you also uncommented the m_pRTCEOutput in the constructor above
            m_pRTCEOutput->data()->setAnnotSet(pRTSE->getAnnotSet());
            m_pRTCEOutput->data()->setSurfSet(pRTSE->getSurfSet());
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

        MatrixXd t_mat = pRTSE->getValue()->data;
        m_pNeuronalConnectivityBuffer->push(&t_mat);
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

            //Init output - Unocmment this if you also uncommented the m_pRTCEOutput in the constructor above
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

            //Check if buffer initialized
            if(!m_pNeuronalConnectivityBuffer) {
                m_pNeuronalConnectivityBuffer = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(64, counter, pRTMSA->getMultiSampleArray()[0].cols()));
            }

        }

        MatrixXd data;
        for(qint32 i = 0; i < pRTMSA->getMultiSampleArray().size(); ++i)
        {
            const MatrixXd& t_mat = pRTMSA->getMultiSampleArray()[i];
            data.resize(m_chIdx.size(), t_mat.cols());

            for(qint32 j = 0; j < m_chIdx.size(); ++j)
            {
                data.row(j) = t_mat.row(m_chIdx.at(j));
            }

            m_pNeuronalConnectivityBuffer->push(&data);
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
        msleep(10);// Wait for fiff Info
    }

    int skip_count = 0;

    while(m_bIsRunning)
    {
        //Dispatch the inputs
        MatrixXd t_mat = m_pNeuronalConnectivityBuffer->pop();

        //Do processing after skip count has reached limit
        if((skip_count % m_iDownSample) == 0)
        {
//            //Do connectivity estimation here
//            QElapsedTimer time;
//            time.start();

            QList<MatrixXd> epochDataList;
            epochDataList.append(t_mat);

            QMutexLocker locker(&m_mutex);

            m_connectivitySettings.m_matDataList = epochDataList;

            Connectivity tConnectivity(m_connectivitySettings);

            //Send the data to the connected plugins and the online display
            //Unocmment this if you also uncommented the m_pRTCEOutput in the constructor above
            m_pRTCEOutput->data()->setValue(tConnectivity.calculateConnectivity());

//            qDebug()<<"----------------------------------------";
//            qDebug()<<"----------------------------------------";
//            qDebug()<<"NeuronalConnectivity::run() - time.elapsed()" << time.elapsed();
//            qDebug()<<"----------------------------------------";
//            qDebug()<<"----------------------------------------";
        }

        ++skip_count;
    }
}


//*************************************************************************************************************

void NeuronalConnectivity::onMetricChanged(const QString& sMetric)
{
    QMutexLocker locker(&m_mutex);

    m_connectivitySettings.m_sConnectivityMethods = QStringList() << sMetric;
}


//*************************************************************************************************************

void NeuronalConnectivity::onWindowTypeChanged(const QString& windowType)
{
    QMutexLocker locker(&m_mutex);

    m_connectivitySettings.m_sWindowType = windowType;
}
