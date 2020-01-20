//=============================================================================================================
/**
 * @file     epidetect.cpp
 * @author   Erik Hornberger <erik.hornberger@shi-g.com>;
 *           Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Louis Eichhorst <Louis.Eichhorst@tu-ilmenau.de>
 * @version  1.0
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Erik Hornberger, Gabriel B Motta, Lorenz Esch, Louis Eichhorst. All rights reserved.
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
 * @brief    Definition of the Epidetect class.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "epidetect.h"
#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace EPIDETECTPLUGIN;
using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace IOBUFFER;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Epidetect::Epidetect()
: m_bIsRunning(false)
, m_pEpidetectInput(NULL)
, m_pEpidetectOutput(NULL)
, m_pEpidetectBuffer(CircularMatrixBuffer<double>::SPtr())
{
    //Add action which will be visible in the plugin's toolbar
    m_pActionShowWidget = new QAction(QIcon(":/images/options.png"), tr(" Toolbar Widget"),this);
    m_pActionShowWidget->setShortcut(tr("F12"));
    m_pActionShowWidget->setStatusTip(tr(" Toolbar Widget"));
    connect(m_pActionShowWidget, &QAction::triggered,
            this, &Epidetect::showWidget);
    addPluginAction(m_pActionShowWidget);
    m_iDim = 3;
    m_dR = 0.3;
    m_iN = 3;
    m_dMargin = 2;
    m_dThreshold1 = 0.75;
    m_dThreshold2 = 0.6;
    m_iListLength = 20;
    m_iFuzzyEnStep = 12;
    m_iChWeight = 15;
    m_dMuGes = 0;
}


//*************************************************************************************************************

Epidetect::~Epidetect()
{
    if(this->isRunning())
        stop();
}


//*************************************************************************************************************

QSharedPointer<IPlugin> Epidetect::clone() const
{
    QSharedPointer<Epidetect> pEpidetectClone(new Epidetect);
    return pEpidetectClone;
}


//*************************************************************************************************************

void Epidetect::init()
{
    // Input
    m_pEpidetectInput = PluginInputData<RealTimeMultiSampleArray>::create(this, "EpidetectIn", "Epidetect input data");
    connect(m_pEpidetectInput.data(), &PluginInputConnector::notify, this, &Epidetect::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pEpidetectInput);

    // Output - Uncomment this if you don't want to send processed data (in form of a matrix) to other plugins.
    // Also, this output stream will generate an online display in  plugin
    m_pEpidetectOutput = PluginOutputData<RealTimeMultiSampleArray>::create(this, "EpidetectOut", "Epidetect output data");
    m_outputConnectors.append(m_pEpidetectOutput);

    //Delete Buffer - will be initailzed with first incoming data
    if(!m_pEpidetectBuffer.isNull())
    {
        m_pEpidetectBuffer = CircularMatrixBuffer<double>::SPtr();
    }
}


//*************************************************************************************************************

void Epidetect::unload()
{

}


//*************************************************************************************************************

bool Epidetect::start()
{
//    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
//    if(this->isRunning())
//    {
//        QThread::wait();
//    }

    m_bIsRunning = true;

    //Start thread
    QThread::start();

    return true;
}


//*************************************************************************************************************

bool Epidetect::stop()
{
    m_bIsRunning = false;

    m_pEpidetectBuffer->releaseFromPop();
    m_pEpidetectBuffer->releaseFromPush();

    m_pEpidetectBuffer->clear();

    return true;
}


//*************************************************************************************************************

IPlugin::PluginType Epidetect::getType() const
{
    return _IAlgorithm;
}


//*************************************************************************************************************

QString Epidetect::getName() const
{
    return "epiDetect";
}


//*************************************************************************************************************

QWidget* Epidetect::setupWidget()
{
    EpidetectSetupWidget* setupWidget = new EpidetectSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}


//*************************************************************************************************************

void Epidetect::update(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<RealTimeMultiSampleArray>();

    if(pRTMSA) {
        //Check if buffer initialized
        if(!m_pEpidetectBuffer) {
            m_pEpidetectBuffer = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(64, pRTMSA->getNumChannels(), pRTMSA->getMultiSampleArray()[0].cols()));
        }

        //Fiff information
        if(!m_pFiffInfo) {
            m_pFiffInfo = pRTMSA->info();

            //Init output - Unocmment this if you also uncommented the m_pEpidetectOutput in the constructor above
            m_pEpidetectOutput->data()->initFromFiffInfo(m_pFiffInfo);
            m_pEpidetectOutput->data()->setMultiArraySize(1);
            m_pEpidetectOutput->data()->setVisibility(true);
        }

        MatrixXd t_mat;

        for(unsigned char i = 0; i < pRTMSA->getMultiArraySize(); ++i) {
            t_mat = pRTMSA->getMultiSampleArray()[i];
            m_pEpidetectBuffer->push(&t_mat);
        }
    }
}



//*************************************************************************************************************


QPair<Eigen::MatrixXd, QList<int> > Epidetect::prepareData(Eigen::MatrixXd mat)
{
    MatrixXd trimmedMatrix;
    QList<int> stimLocations;
    QStringList badChs = m_pFiffInfo->bads;
    QStringList chNames = m_pFiffInfo->ch_names;
    trimmedMatrix.resize(mat.rows()- badChs.size(), mat.cols());

    int j = 0;

    for (int i=0 ; i < mat.rows(); i++)
    {
        QString type = m_pFiffInfo->channel_type(i);

        if (type == "stim")
            stimLocations << i;
        if (!(badChs.contains(chNames[i])))
        {
            if ((type != "stim") && (type != "ecg") && (type != "eeg"))
            {
                trimmedMatrix.row(j) = mat.row(i);
                j++;

            }
        }

    }

    qSort(stimLocations);

    trimmedMatrix.conservativeResize(j,trimmedMatrix.cols());
    QPair<MatrixXd, QList<int> > out;
    out.first = trimmedMatrix;
    out.second = stimLocations;

    return out;

}


//*************************************************************************************************************

void Epidetect::run()
{
    bool overlap = false;
    CalcMetric calculator;
    FuzzyMembership P2P;
    FuzzyMembership Kurtosis;
    FuzzyMembership FuzzyEn;

    while(!m_pFiffInfo)
        msleep(10);// Wait for fiff Info

    MatrixXd firstHalfTrimmed;
    MatrixXd lastHalfTrimmed;
    MatrixXd trimmedData;
    QList<int> stimChs;
    MatrixXd t_mat;
    MatrixXd FuzzyEnHistoryValues;
    MatrixXd KurtosisHistoryValues;
    MatrixXd P2PHistoryValues;
    int counter = 0;

    while(m_bIsRunning)
    {
        QElapsedTimer timer;
        QPair<MatrixXd,QList<int> > data;

        m_dMuGes = 0;

        //Dispatch the inputs
        if (!overlap)
        {
            t_mat = m_pEpidetectBuffer->pop();
            data = prepareData(t_mat);
            trimmedData = data.first;
            stimChs = data.second;
            t_mat.row(stimChs[0]).setZero();

        }

        timer.start();
        MatrixXd window;

        if (!overlap)
        {
            firstHalfTrimmed = trimmedData.block(0, 0, trimmedData.rows(), (trimmedData.cols()/2));
            lastHalfTrimmed = trimmedData.block(0,(trimmedData.cols()/2), trimmedData.rows(), (trimmedData.cols()/2));
            overlap = true;
        }
        else
        {
            firstHalfTrimmed = lastHalfTrimmed;
            lastHalfTrimmed = trimmedData.block(0, 0, trimmedData.rows(), (trimmedData.cols()/2));
            overlap = false;
        }

        window.resize(firstHalfTrimmed.rows(), (firstHalfTrimmed.cols()+lastHalfTrimmed.cols()));
        window.block(0,0, firstHalfTrimmed.rows(), firstHalfTrimmed.cols()) = firstHalfTrimmed;
        window.block(0, firstHalfTrimmed.cols(), lastHalfTrimmed.rows(), lastHalfTrimmed.cols()) = lastHalfTrimmed;

        if (KurtosisHistoryValues.rows() != window.rows())
        {
            KurtosisHistoryValues.conservativeResize(window.rows(), 3);
            P2PHistoryValues.conservativeResize(window.rows(), 3);
            FuzzyEnHistoryValues.conservativeResize(window.rows(), 3);
        }

        calculator.m_iListLength = m_iListLength;
        calculator.m_iFuzzyEnStep = m_iFuzzyEnStep;
        calculator.calcAll(window, m_iDim, m_dR , m_iN);
        MatrixXd mu;
        MatrixXd p2pHistory =calculator.getP2PHistory();
        MatrixXd kurtosisHistory = calculator.getKurtosisHistory();
        MatrixXd fuzzyEnHistory = calculator.getFuzzyEnHistory();
        VectorXd newP2PVal = calculator.getP2P();
        VectorXd newKurtosisVal = calculator.getKurtosis();

        if (counter == m_iFuzzyEnStep*m_iListLength-1)
        {
            FuzzyEnHistoryValues.col(0) = fuzzyEnHistory.rowwise().minCoeff();
            FuzzyEnHistoryValues.col(1) = fuzzyEnHistory.rowwise().mean();
            FuzzyEnHistoryValues.col(2) = fuzzyEnHistory.rowwise().maxCoeff();
        }

        if (counter % m_iListLength == m_iListLength - 1)
        {
            KurtosisHistoryValues.col(0) = kurtosisHistory.rowwise().minCoeff();
            KurtosisHistoryValues.col(1) = kurtosisHistory.rowwise().mean();
            KurtosisHistoryValues.col(2) = kurtosisHistory.rowwise().maxCoeff();
            P2PHistoryValues.col(0) = p2pHistory.rowwise().minCoeff();
            P2PHistoryValues.col(1) = p2pHistory.rowwise().mean();
            P2PHistoryValues.col(2) = p2pHistory.rowwise().maxCoeff();


        }

        if (calculator.m_bHistoryReady)
        {
            m_dvecMuP2P = P2P.getMembership(p2pHistory, P2PHistoryValues, newP2PVal, m_dvecEpiHistory, m_dMargin, 'r');
            m_dvecMuKurtosis = Kurtosis.getMembership(kurtosisHistory, KurtosisHistoryValues, newKurtosisVal, m_dvecEpiHistory, m_dMargin, 'm'); //TODO: check whether it really is 'm'
            mu.resize(0,0);
            mu.resize(window.rows(),2);
            mu.col(0)=m_dvecMuP2P;
            mu.col(1)=m_dvecMuKurtosis;
            m_dvecMuMin = mu.rowwise().minCoeff();

            if (m_dvecMuMin.maxCoeff() > m_dThreshold1)
            {
                QList<int> checkChs;
                for (int i= 0; i < m_dvecMuMin.rows(); i++)
                {
                    if (m_dvecMuMin(i) > m_dThreshold1)
                        checkChs << i;
                }

                VectorXd newFuzzyEnVal = calculator.onSeizureDetection(m_iDim ,m_dR, m_iN, checkChs);
                m_dvecMuFuzzyEn = FuzzyEn.getMembership(fuzzyEnHistory, FuzzyEnHistoryValues, newFuzzyEnVal, m_dvecEpiHistory,  m_dMargin, 'l' );
                mu.col(0)= mu.rowwise().mean();
                mu.col(1)= m_dvecMuFuzzyEn;

                for(int i = 0; i < checkChs.length(); i++)
                {
                    m_dMuGes = m_dMuGes + (m_dvecMuFuzzyEn(checkChs[i]));
                }

                m_dMuGes = m_dMuGes/(checkChs.length() + m_iChWeight);

                if (m_dMuGes < m_dThreshold2)
                    m_dMuGes = 0;

                if (overlap)
                {
                    t_mat(stimChs[0], 1) =  m_dMuGes;

                }
                else
                {
                    t_mat(stimChs[0], (t_mat.cols()/2)) = m_dMuGes;
                }
            }
        }

        if (overlap)
            m_pEpidetectOutput->data()->setValue(t_mat);
        std::cout << timer.elapsed() << " ms \n";
    }
}


//*************************************************************************************************************

void Epidetect::showWidget()
{
    m_pWidget = EpidetectWidget::SPtr(new EpidetectWidget(m_iDim, m_dR, m_iN, m_dMargin, m_dThreshold1, m_dThreshold2, m_iListLength, m_iFuzzyEnStep, m_iChWeight));

    connect(m_pWidget.data(), &EpidetectWidget::newValues, this, &Epidetect::updateValues);

    m_pWidget->show();
}


//*************************************************************************************************************

void Epidetect::updateValues()
{
    m_iDim = m_pWidget->m_iDimVal;
    m_dR = m_pWidget->m_dRVal;
    m_iN = m_pWidget->m_iNVal;
    m_dMargin = m_pWidget->m_dMarginVal;
    m_dThreshold1 = m_pWidget->m_dThreshold1Val;
    m_dThreshold2 = m_pWidget->m_dThreshold2Val;
    m_iListLength = m_pWidget->m_iListLengthVal;
    m_iFuzzyEnStep = m_pWidget->m_iFuzzyEnStepVal;
    m_iChWeight = m_pWidget->m_iChWeight;

}
