//=============================================================================================================
/**
* @file     bci.cpp
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     December, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Contains the implementation of the BCI class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bci.h"

#include "FormFiles/bcisetupwidget.h"
#include "FormFiles/bcifeaturewindow.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
#include <QtCore/QTextStream>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BCIPlugin;
using namespace std;
using namespace UTILSLIB;

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BCI::BCI()
: m_qStringResourcePath(qApp->applicationDirPath()+"/mne_x_plugins/resources/bci/"),
  m_bProcessData(false)
{
    QString path("filterOutput.txt");
    path.prepend(m_qStringResourcePath);

    m_outStreamDebug.open(path.toStdString(), ios::trunc);
}


//*************************************************************************************************************

BCI::~BCI()
{
    //std::cout << "BCI::~BCI() " << std::endl;

    //If the program is closed while the sampling is in process
    if(this->isRunning())
        this->stop();
}


//*************************************************************************************************************

QSharedPointer<IPlugin> BCI::clone() const
{
    QSharedPointer<BCI> pTMSIClone(new BCI());
    return pTMSIClone;
}


//*************************************************************************************************************

void BCI::init()
{
    m_bIsRunning = false;

    // Inputs - Source estimates and sensor level
    m_pRTSEInput = PluginInputData<RealTimeSourceEstimate>::create(this, "BCIInSource", "BCI source input data");
    connect(m_pRTSEInput.data(), &PluginInputConnector::notify, this, &BCI::updateSource, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTSEInput);

    m_pRTMSAInput = PluginInputData<NewRealTimeMultiSampleArray>::create(this, "BCIInSensor", "SourceLab sensor input data");
    connect(m_pRTMSAInput.data(), &PluginInputConnector::notify, this, &BCI::updateSensor, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTMSAInput);

    // Output
    m_pBCIOutput = PluginOutputData<NewRealTimeSampleArray>::create(this, "ControlSignal", "BCI output data");
    m_outputConnectors.append(m_pBCIOutput);

    //m_pBCIOutput->data()->setMaxValue();

    //Delete Buffer - will be initailzed with first incoming data
    if(!m_pBCIBuffer_Sensor.isNull())
        m_pBCIBuffer_Sensor = CircularMatrixBuffer<double>::SPtr();

    if(!m_pBCIBuffer_Source.isNull())
        m_pBCIBuffer_Source = CircularMatrixBuffer<double>::SPtr();

    // Delete fiff info because the initialisation of the fiff info is seen as the first data acquisition from the input stream
    if(!m_pFiffInfo_Sensor.isNull())
        m_pFiffInfo_Sensor = FiffInfo::SPtr();

    // Intitalise GUI stuff
    m_bUseSensorData = true;
    m_bUseSourceData = false;
    m_bUseArtefactThresholdReduction = false;
    m_dSlidingWindowSize = 1.0;
    m_dBaseLineWindowSize = 1.0;
    m_dTimeBetweenWindows = 0.04;
    m_iNumberSubSignals = 1;
    m_sSensorBoundaryPath = m_qStringResourcePath;
    m_sSourceBoundaryPath = m_qStringResourcePath;

    // Intitalise feature selection
    m_slChosenFeatureSensor << "LA4" << "RA4"; //<< "TEST";

    // Initialise boundaries with linear coefficients y = mx+c -> vector = [m c] -> default [1 0]
    m_vLoadedSensorBoundary.push_back(1);
    m_vLoadedSensorBoundary.push_back(0);

    m_vLoadedSourceBoundary.push_back(1);
    m_vLoadedSourceBoundary.push_back(0);

    // Initalise sliding window stuff
    m_iTBWIndexSensor = 0;
    m_bFillSensorWindowFirstTime = true;

    // Initialise filter stuff
    if(!m_filterOperator.isNull())
        m_filterOperator = QSharedPointer<FilterData>(new FilterData());

    m_dFilterLowerBound = 7.0;
    m_dFilterUpperBound = 14.0;
    m_dParcksWidth = (m_dFilterUpperBound-m_dFilterLowerBound)/2;
    m_iFilterOrder = 100;

    BCIFeatureWindow* featureWidget = new BCIFeatureWindow(this);
    featureWidget->initGui();
    featureWidget->show();
}


//*************************************************************************************************************

bool BCI::start()
{
    m_bIsRunning = true;

    QThread::start();

    return true;
}


//*************************************************************************************************************

bool BCI::stop()
{
    //Wait until this thread (BCI) is stopped
    m_bIsRunning = false;

    // Get data buffers out of idle state if they froze in the acquire or release function
    //In case the semaphore blocks the thread -> Release the QSemaphore and let it exit from the pop function (acquire statement)

    if(m_bProcessData) // Only clear if buffers have been initialised
    {
        m_pBCIBuffer_Sensor->releaseFromPop();
        m_pBCIBuffer_Sensor->releaseFromPush();
        //    m_pBCIBuffer_Source->releaseFromPop();
        //    m_pBCIBuffer_Source->releaseFromPush();
    }

    // Stop filling buffers with data from the inputs
    m_bProcessData = false;

    // Clear stream
    m_outStreamDebug.close();
    m_outStreamDebug.clear();

    // Delete all features
    m_lFeaturesSensor.clear();

    return true;
}


//*************************************************************************************************************

IPlugin::PluginType BCI::getType() const
{
    return _IAlgorithm;
}


//*************************************************************************************************************

QString BCI::getName() const
{
    return "BCI EEG";
}


//*************************************************************************************************************

QWidget* BCI::setupWidget()
{
    BCISetupWidget* setupWidget = new BCISetupWidget(this);//widget is later destroyed by CentralWidget - so it has to be created everytime new

    //init properties dialog
    setupWidget->initGui();

    return setupWidget;
}


//*************************************************************************************************************

void BCI::updateSensor(XMEASLIB::NewMeasurement::SPtr pMeasurement)
{
    QSharedPointer<NewRealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<NewRealTimeMultiSampleArray>();
    if(pRTMSA)
    {
        //Check if buffer initialized
        if(!m_pBCIBuffer_Sensor)
            m_pBCIBuffer_Sensor = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(64, pRTMSA->getNumChannels(), pRTMSA->getMultiArraySize()));

        //Fiff information for sensor level
        if(!m_pFiffInfo_Sensor)
        {
            m_pFiffInfo_Sensor = pRTMSA->getFiffInfo();

            // Adjust working matrixes (sliding window and time between windows matrix) size so that the samples from the tmsi plugin stream fit in the matrix perfectly
            int modulo = (int)(m_pFiffInfo_Sensor->sfreq*m_dSlidingWindowSize) % (int)pRTMSA->getMultiArraySize();

            int rows = m_slChosenFeatureSensor.size();
            int cols = m_pFiffInfo_Sensor->sfreq*m_dSlidingWindowSize-modulo;
            m_matSlidingWindowSensor.resize(rows, cols);

            modulo = (int)(m_pFiffInfo_Sensor->sfreq*m_dTimeBetweenWindows) % (int)pRTMSA->getMultiArraySize();
            rows = m_slChosenFeatureSensor.size();
            cols = m_pFiffInfo_Sensor->sfreq*m_dTimeBetweenWindows-modulo;
            m_matTimeBetweenWindowsSensor.resize(rows, cols);

            // Build filter operator
            double dCenterFreqNyq = (m_dFilterLowerBound+((m_dFilterUpperBound - m_dFilterLowerBound)/2))/(m_pFiffInfo_Sensor->sfreq/2);
            double dBandwidthNyq = (m_dFilterUpperBound - m_dFilterLowerBound)/(m_pFiffInfo_Sensor->sfreq/2);
            double dParksWidth = m_dParcksWidth/(m_pFiffInfo_Sensor->sfreq/2);

            m_filterOperator = QSharedPointer<FilterData>(new FilterData(QString("BPF"),FilterData::BPF,m_iFilterOrder,dCenterFreqNyq,dBandwidthNyq,dParksWidth,1024)); // letztes Argument muss 2er potenz sein - fft länge

            //m_filterOperator = QSharedPointer<FilterData>(new FilterData(QString("BPF"),FilterData::BPF,100,100/512,25/512,0.1,2048)); // letztes Argument muss 2er potenz sein - fft länge

            for(int i = 0; i<m_filterOperator->m_dCoeffA.cols(); i++)
                m_outStreamDebug << m_filterOperator->m_dFFTCoeffA(0,i).real() <<"+" << m_filterOperator->m_dFFTCoeffA(0,i).imag() << "i "  << endl;

            m_outStreamDebug << endl << endl;

            for(int i = 0; i<m_filterOperator->m_dCoeffA.cols(); i++)
                m_outStreamDebug << m_filterOperator->m_dCoeffA(0,i) << endl;


            m_outStreamDebug << "---------------------------------------------------------------------" << endl;
        }

        if(m_bProcessData)
        {
            MatrixXd t_mat(pRTMSA->getNumChannels(), pRTMSA->getMultiArraySize());

            for(unsigned char i = 0; i < pRTMSA->getMultiArraySize(); ++i)
                t_mat.col(i) = pRTMSA->getMultiSampleArray()[i];

            m_pBCIBuffer_Sensor->push(&t_mat);
        }
    }
}


//*************************************************************************************************************

void BCI::updateSource(XMEASLIB::NewMeasurement::SPtr pMeasurement)
{
    QSharedPointer<RealTimeSourceEstimate> pRTSE = pMeasurement.dynamicCast<RealTimeSourceEstimate>();
    if(pRTSE)
    {
        //Check if buffer initialized
        if(!m_pBCIBuffer_Source)
            m_pBCIBuffer_Source = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(64, pRTSE->getValue().size(), pRTSE->getArraySize()));

        if(m_bProcessData)
        {
            MatrixXd t_mat(pRTSE->getValue().size(), pRTSE->getArraySize());

            for(unsigned char i = 0; i < pRTSE->getArraySize(); ++i)
                t_mat.col(i) = pRTSE->getStc().data.col(i);

            m_pBCIBuffer_Source->push(&t_mat);
        }
    }
}


//*************************************************************************************************************

void BCI::applyFilterOperatorConcurrently(QPair<int, RowVectorXd> &chdata)
{
    chdata.second = m_filterOperator->applyFFTFilter(chdata.second);
}


//*************************************************************************************************************

QPair<int,QVector<double>> BCI::applyFeatureCalcConcurrentlyOnSensorLevel(const QPair<int, RowVectorXd> &chdata)
{
    RowVectorXd data = chdata.second;
    QVector<double> features;

    features.push_back(data.squaredNorm()); // Compute variance

    return QPair<int,QVector<double>>(chdata.first, features);
}


//*************************************************************************************************************

void BCI::run()
{
    while(m_bIsRunning)
    {
        // Wait for fiff Info if not yet received - this is needed because we have to wait until the buffers are firstly initiated in the update functions
        while(!m_pFiffInfo_Sensor)
            msleep(10);

        // Start filling buffers with data from the inputs
        m_bProcessData = true;

        // Sensor level: Fill matrices with data
        if(m_bFillSensorWindowFirstTime) // Sensor level: Fill m_matSlidingWindowSensor with data for the first time
        {
            if(m_iTBWIndexSensor < m_matSlidingWindowSensor.cols())
            {
                MatrixXd t_mat = m_pBCIBuffer_Sensor->pop();

                // Get only the rows from the matrix which correspond with the selected features, namely electrodes on sensor level and destrieux clustered regions on source level
                for(int i = 0; i < m_matSlidingWindowSensor.rows(); i++)
                    m_matSlidingWindowSensor.block(i, m_iTBWIndexSensor, 1, t_mat.cols()) = t_mat.block(m_mapElectrodePinningScheme[m_slChosenFeatureSensor.at(i)], 0, 1, t_mat.cols());

                m_iTBWIndexSensor = m_iTBWIndexSensor + t_mat.cols();
            }
            else // m_matSlidingWindowSensor is full for the first time
            {
                m_iTBWIndexSensor = 0;
                m_bFillSensorWindowFirstTime = false;
            }
        }
        else // Sensor level: Fill m_matTimeBetweenWindowsSensor matrix until full -> Then -> recalculate m_matSlidingWindowSensor, calculate features and classify
        {
            if(m_iTBWIndexSensor < m_matTimeBetweenWindowsSensor.cols())
            {
                MatrixXd t_mat = m_pBCIBuffer_Sensor->pop();

                // Get only the rows from the matrix which correspond with the selected features, namely electrodes on sensor level and destrieux clustered regions on source level
                for(int i = 0; i < m_matTimeBetweenWindowsSensor.rows(); i++)
                    m_matTimeBetweenWindowsSensor.block(i, m_iTBWIndexSensor, 1, t_mat.cols()) = t_mat.block(m_mapElectrodePinningScheme[m_slChosenFeatureSensor.at(i)], 0, 1, t_mat.cols());

                m_iTBWIndexSensor = m_iTBWIndexSensor + t_mat.cols();
            }
            else // Recalculate m_matSlidingWindowSensor -> Calculate features, classify and store results
            {
                // Recalculate m_matSlidingWindowSensor -> Delete same block size of data from the right -> use eval() to resolve Eigens aliasing problem
                m_matSlidingWindowSensor.block(0, 0, m_matSlidingWindowSensor.rows(), m_matSlidingWindowSensor.cols()-m_matTimeBetweenWindowsSensor.cols()) = m_matSlidingWindowSensor.block(0, m_matTimeBetweenWindowsSensor.cols(), m_matSlidingWindowSensor.rows(), m_matSlidingWindowSensor.cols()-m_matTimeBetweenWindowsSensor.cols()).eval();

                // Recalculate m_matSlidingWindowSensor -> push m_matTimeBetweenWindowsSensor from the left
                m_matSlidingWindowSensor.block(0, m_matSlidingWindowSensor.cols()-m_matTimeBetweenWindowsSensor.cols(), m_matTimeBetweenWindowsSensor.rows(), m_matTimeBetweenWindowsSensor.cols()) = m_matTimeBetweenWindowsSensor;

                // Filter data in m_matSlidingWindowSensor concurrently using map()
                QList<QPair<int,RowVectorXd>> filteredRows;
                for(int i = 0; i< m_matSlidingWindowSensor.rows(); i++)
                    filteredRows << QPair<int,RowVectorXd>(i, m_matSlidingWindowSensor.row(i));

                QFuture<void> future = QtConcurrent::map(filteredRows,[this](QPair<int,RowVectorXd>& chdata) {
                    applyFilterOperatorConcurrently(chdata);
                });

                future.waitForFinished();

//                for(int i=0; i<m_matSlidingWindowSensor.rows(); i++)
//                    m_outStreamDebug << "m_matSlidingWindowSensor at row " << i << ": " << m_matSlidingWindowSensor.row(i) << "\n";

//                m_outStreamDebug<<endl;

//                for(int i=0; i<filteredRows.size(); i++)
//                    m_outStreamDebug << "filteredRows at row " << i << ": " << filteredRows.at(i).second << "\n";

//                m_outStreamDebug << endl << "---------------------------------------------------------" << endl << endl;

                // Calculate features concurrently using mapped()
                std::function<QPair<int,QVector<double>> (QPair<int,RowVectorXd>&)> applyOps = [this](QPair<int,RowVectorXd>& chdata) -> QPair<int,QVector<double>> {
                    return applyFeatureCalcConcurrentlyOnSensorLevel(chdata);
                };

                QFuture<QPair<int,QVector<double>>> futureCalculatedFeatures = QtConcurrent::mapped(filteredRows.begin(), filteredRows.end(), applyOps);

                futureCalculatedFeatures.waitForFinished();

                // Store features
                m_lFeaturesSensor.append(futureCalculatedFeatures.results());

                emit paintFeatures();

                // TODO: Classify features and store results concurrently using map()

                // TODO: Check how full the classification result vector is -> if too big emit signal and average results, delete all results and make final decision to triggerbox

                // Test if data is correctly streamed to this plugin
//                cout<<"Recalculate matrix"<<endl;
//                for(int i = 1; i<m_matSlidingWindowSensor.cols() ; i++)
//                {
//                    cout<<m_matSlidingWindowSensor(2,i)<<endl;
////                    if(m_matSlidingWindowSensor(2,i) - m_matSlidingWindowSensor(2,i-1) != 1)
////                        cout<<"Sequence error while streaming from tmsi plugin at position: "<<i<<endl;
//                }

                m_iTBWIndexSensor = 0;
            }
        }
    }
}
