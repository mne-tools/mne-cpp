//=============================================================================================================
/**
 * @file     bci.cpp
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @version  dev
 * @date     December, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Lars Debor, Christoph Dinh, Lorenz Esch, Viktor Klueber. All rights reserved.
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

using namespace BCIPLUGIN;
using namespace std;
using namespace UTILSLIB;
using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace IOBUFFER;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BCI::BCI()
: m_qStringResourcePath(qApp->applicationDirPath()+"/resources/mne_scan/plugins/bci/")
, m_bProcessData(false)
{
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
    m_bTriggerActivated = false;

    // Inputs - Source estimates and sensor level
    m_pRTSEInput = PluginInputData<RealTimeSourceEstimate>::create(this, "BCIInSource", "BCI source input data");
    connect(m_pRTSEInput.data(), &PluginInputConnector::notify, this, &BCI::updateSource, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTSEInput);

    m_pRTMSAInput = PluginInputData<RealTimeMultiSampleArray>::create(this, "BCIInSensor", "SourceLab sensor input data");
    connect(m_pRTMSAInput.data(), &PluginInputConnector::notify, this, &BCI::updateSensor, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTMSAInput);

    // Output streams
    m_pBCIOutputOne = PluginOutputData<RealTimeSampleArray>::create(this, "ControlSignal", "BCI output data One");
    m_pBCIOutputOne->data()->setArraySize(1);
    m_pBCIOutputOne->data()->setName("Boundary");
    m_outputConnectors.append(m_pBCIOutputOne);

    m_pBCIOutputTwo = PluginOutputData<RealTimeSampleArray>::create(this, "ControlSignal", "BCI output data Two");
    m_pBCIOutputTwo->data()->setArraySize(1);
    m_pBCIOutputTwo->data()->setName("Left electrode var");
    m_outputConnectors.append(m_pBCIOutputTwo);

    m_pBCIOutputThree = PluginOutputData<RealTimeSampleArray>::create(this, "ControlSignal", "BCI output data Three");
    m_pBCIOutputThree->data()->setArraySize(1);
    m_pBCIOutputThree->data()->setName("Right electrode var");
    m_outputConnectors.append(m_pBCIOutputThree);

    m_pBCIOutputFour = PluginOutputData<RealTimeSampleArray>::create(this, "ControlSignal", "BCI output data Four");
    m_pBCIOutputFour->data()->setArraySize(1);
    m_pBCIOutputFour->data()->setName("Left electrode");
    m_outputConnectors.append(m_pBCIOutputFour);

    m_pBCIOutputFive = PluginOutputData<RealTimeSampleArray>::create(this, "ControlSignal", "BCI output data Five");
    m_pBCIOutputFive->data()->setArraySize(1);
    m_pBCIOutputFive->data()->setName("Right electrode");
    m_outputConnectors.append(m_pBCIOutputFive);

    // Delete Buffer - will be initailzed with first incoming data
    m_pBCIBuffer_Sensor = CircularMatrixBuffer<double>::SPtr();
    m_pBCIBuffer_Source = CircularMatrixBuffer<double>::SPtr();

    // Delete fiff info because the initialisation of the fiff info is seen as the first data acquisition from the input stream
    m_pFiffInfo_Sensor = FiffInfo::SPtr();

    // Intitalise GUI stuff
    m_bSubtractMean = true;
    m_bUseFilter = true;
    m_bUseSensorData = true;
    m_bUseSourceData = false;
    m_bDisplayFeatures = true;
    m_bUseArtefactThresholdReduction = false;
    m_dSlidingWindowSize = 0.5;
    m_dTimeBetweenWindows = 0.5;
    m_dDisplayRangeBoundary = 15;
    m_dDisplayRangeVariances = 5;
    m_dDisplayRangeElectrodes = 7;
    m_iNumberFeatures = 1;
    m_dThresholdValue = 30;
    m_iNumberFeaturesToDisplay = 30;
    m_iFeatureCalculationType = 0;

    // Intitalise feature selection
    m_slChosenFeatureSensor << "LA4" << "RA4"; //<< "TEST";

    // Initalise sliding window stuff
    m_bFillSensorWindowFirstTime = true;

    // Initialise filter stuff
    m_filterOperator = QSharedPointer<FilterData>(new FilterData());

    m_dFilterLowerBound = 7.0;
    m_dFilterUpperBound = 14.0;
    m_dParcksWidth = m_dFilterLowerBound-1; // (m_dFilterUpperBound-m_dFilterLowerBound)/2;
    m_iFilterOrder = 256;

    // Init BCIFeatureWindow for visualization
    m_BCIFeatureWindow = QSharedPointer<BCIFeatureWindow>(new BCIFeatureWindow(this));
}


//*************************************************************************************************************

void BCI::unload()
{

}


//*************************************************************************************************************

bool BCI::start()
{
    // Do not start BCI if the number of chosen electrodes/features is uneven
    if(m_slChosenFeatureSensor.size()%2 != 0)
    {
        if(!m_slChosenFeatureSensor.contains("TEST"))
        {
            QMessageBox msgBox;
            msgBox.setText("The number of selected electrodes needs to be even.");
            msgBox.setInformativeText("Please select feautres again");
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
            return false;
        }
    }

    // Initialise index
    m_iTBWIndexSensor = 0;
    m_iNumberOfCalculatedFeatures = 0;

    // BCIFeatureWindow show and init
    if(m_bDisplayFeatures)
    {
        m_BCIFeatureWindow->initGui();
        m_BCIFeatureWindow->show();
    }

    // Init debug output stream
    QString path("BCIDebugFile.txt");
    path.prepend(m_qStringResourcePath);
    m_outStreamDebug.open(path.toStdString(), ios::trunc);

    m_pFiffInfo_Sensor = FiffInfo::SPtr();

    m_bFillSensorWindowFirstTime = true;

//    // Set display ranges for output channels
//    m_pBCIOutputOne->data()->setMaxValue(m_dDisplayRangeBoundary);
//    m_pBCIOutputOne->data()->setMinValue(-m_dDisplayRangeBoundary);

//    m_pBCIOutputTwo->data()->setMaxValue(m_dDisplayRangeVariances);
//    m_pBCIOutputTwo->data()->setMinValue(0);

//    m_pBCIOutputThree->data()->setMaxValue(m_dDisplayRangeVariances);
//    m_pBCIOutputThree->data()->setMinValue(0);

//    m_pBCIOutputFour->data()->setMaxValue(m_dDisplayRangeElectrodes);
//    m_pBCIOutputFour->data()->setMinValue(-m_dDisplayRangeElectrodes);

//    m_pBCIOutputFive->data()->setMaxValue(m_dDisplayRangeElectrodes);
//    m_pBCIOutputFive->data()->setMinValue(-m_dDisplayRangeElectrodes);

    // variance
    m_pBCIOutputOne->data()->setMaxValue(5);
    m_pBCIOutputOne->data()->setMinValue(-5);

    m_pBCIOutputTwo->data()->setMaxValue(10);
    m_pBCIOutputTwo->data()->setMinValue(0);

    m_pBCIOutputThree->data()->setMaxValue(10);
    m_pBCIOutputThree->data()->setMinValue(0);

    m_pBCIOutputFour->data()->setMaxValue(1e-05);
    m_pBCIOutputFour->data()->setMinValue(-1e-05);

    m_pBCIOutputFive->data()->setMaxValue(1e-05);
    m_pBCIOutputFive->data()->setMinValue(-1e-05);

//    // log variance
//    m_pBCIOutputOne->data()->setMaxValue(10);
//    m_pBCIOutputOne->data()->setMinValue(-10);

//    m_pBCIOutputTwo->data()->setMaxValue(15);
//    m_pBCIOutputTwo->data()->setMinValue(0);

//    m_pBCIOutputThree->data()->setMaxValue(15);
//    m_pBCIOutputThree->data()->setMinValue(0);

//    m_pBCIOutputFour->data()->setMaxValue(10e-04);
//    m_pBCIOutputFour->data()->setMinValue(-10e-04);

//    m_pBCIOutputFive->data()->setMaxValue(10e-04);
//    m_pBCIOutputFive->data()->setMinValue(-10e-04);

    m_bIsRunning = true;

    QThread::start();

    return true;
}


//*************************************************************************************************************

bool BCI::stop()
{
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

    // Reset trigger
    m_bTriggerActivated = false;

    // Clear stream
    m_outStreamDebug.close();
    m_outStreamDebug.clear();

    // Hide feature visualization window
    if(m_bDisplayFeatures)
        m_BCIFeatureWindow->hide();

    // Delete all features and classification results
    clearFeatures();
    clearClassifications();

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

void BCI::updateSensor(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<RealTimeMultiSampleArray>();
    if(pRTMSA)
    {
        //Check if buffer initialized
        if(!m_pBCIBuffer_Sensor)
            m_pBCIBuffer_Sensor = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(64, pRTMSA->getNumChannels(), pRTMSA->getMultiArraySize()));

        // Load Fiff information on sensor level
        if(!m_pFiffInfo_Sensor)
        {
            m_pFiffInfo_Sensor = pRTMSA->info();

            // Adjust working matrixes (sliding window and time between windows matrix) size so that the samples from the tmsi plugin stream fit in the matrix perfectly
            int arraySize = pRTMSA->getMultiArraySize();
            int modulo = int(m_pFiffInfo_Sensor->sfreq*m_dSlidingWindowSize) % arraySize;
            int rows = m_slChosenFeatureSensor.size();
            int cols = m_pFiffInfo_Sensor->sfreq*m_dSlidingWindowSize-modulo;
            m_matSlidingWindowSensor.resize(rows, cols);

            m_matStimChannelSensor.resize(1,cols);

            modulo = int(m_pFiffInfo_Sensor->sfreq*m_dTimeBetweenWindows) % arraySize;
            rows = m_slChosenFeatureSensor.size();
            cols = m_pFiffInfo_Sensor->sfreq*m_dTimeBetweenWindows-modulo;
            m_matTimeBetweenWindowsSensor.resize(rows, cols);

            m_matTimeBetweenWindowsStimSensor.resize(1,cols);

            // Build filter operator
            double dCenterFreqNyq = (m_dFilterLowerBound+((m_dFilterUpperBound - m_dFilterLowerBound)/2))/(m_pFiffInfo_Sensor->sfreq/2);
            double dBandwidthNyq = (m_dFilterUpperBound - m_dFilterLowerBound)/(m_pFiffInfo_Sensor->sfreq/2);
            double dParksWidth = m_dParcksWidth/(m_pFiffInfo_Sensor->sfreq/2);

//            // Calculate needed fft length
//            int exponent = ceil(log10(m_matSlidingWindowSensor.cols())/log10(2));
//            int fftLength = pow(2,exponent+1);

            // Initialise filter operator
            m_filterOperator = QSharedPointer<FilterData>(new FilterData(QString("BPF"),FilterData::BPF,m_iFilterOrder,dCenterFreqNyq,dBandwidthNyq,dParksWidth,m_matSlidingWindowSensor.cols()+m_iFilterOrder)); // letztes Argument muss 2er potenz sein - fft länge

            // Write filter coefficients to debug file
            for(int i = 0; i<m_filterOperator->m_dCoeffA.cols(); i++)
                m_outStreamDebug << m_filterOperator->m_dFFTCoeffA(0,i).real() <<"+" << m_filterOperator->m_dFFTCoeffA(0,i).imag() << "i "  << endl;

            m_outStreamDebug << endl << endl;

            for(int i = 0; i<m_filterOperator->m_dCoeffA.cols(); i++)
                m_outStreamDebug << m_filterOperator->m_dCoeffA(0,i) << endl;

            m_outStreamDebug << "---------------------------------------------------------------------" << endl;
        }

        // Only process data when fiff info has been initialised in run() method
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

void BCI::updateSource(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    cout<<"update source"<<endl;
    QSharedPointer<RealTimeSourceEstimate> pRTSE = pMeasurement.dynamicCast<RealTimeSourceEstimate>();
    if(pRTSE && !pRTSE->getValue().isEmpty())
    {
        //Check if buffer initialized
        if(!m_pBCIBuffer_Source)
            m_pBCIBuffer_Source = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(64, pRTSE->getValue().first()->data.rows(), pRTSE->getValue().first()->data.cols()));

        if(m_bProcessData)
        {
            MatrixXd t_mat(pRTSE->getValue().first()->data.rows(), pRTSE->getValue().first()->data.cols());

            for(unsigned char i = 0; i < pRTSE->getValue().first()->data.cols(); ++i)
                t_mat.col(i) = pRTSE->getValue().first()->data.col(i);

            m_pBCIBuffer_Source->push(&t_mat);
        }
    }
}


//*************************************************************************************************************

void BCI::applyMeanCorrectionConcurrently(QPair<int, RowVectorXd> &chdata)
{
    chdata.second = chdata.second.array() - chdata.second.mean(); // Row vector needs to be transformed to a array in order to subract a scalar
}


//*************************************************************************************************************

void BCI::applyFilterOperatorConcurrently(QPair<int, RowVectorXd> &chdata)
{
    chdata.second = m_filterOperator->applyFFTFilter(chdata.second);
}


//*************************************************************************************************************

QPair< int,QList<double> > BCI::applyFeatureCalcConcurrentlyOnSensorLevel(const QPair<int, RowVectorXd> &chdata)
{
    RowVectorXd data = chdata.second;
    QList<double> features;

    // TODO: Divide into subsignals
    switch(m_iFeatureCalculationType)
    {
        case 0:
            features << data.squaredNorm(); // Compute variance
            break;
        case 1:
            features << std::abs(log10(data.squaredNorm())); // Compute log of variance
            break;
        default:
            features << data.squaredNorm(); // Compute variance
            break;
    }

    return QPair< int,QList<double> >(chdata.first, features);
}


//*************************************************************************************************************

double BCI::applyClassificationCalcConcurrentlyOnSensorLevel(QList<double> &featData)
{
   return classificationBoundaryValue(featData);
}


//*************************************************************************************************************

double BCI::classificationBoundaryValue(const QList<double> &featData)
{
    double return_val = 0;

    if(featData.size() == m_vLoadedSensorBoundary[1].size())
    {
        VectorXd feat_temp(featData.size());
        for(int i = 0; i<featData.size(); i++)
            feat_temp(i) = featData.at(i);

        return_val = m_vLoadedSensorBoundary[0](0) + m_vLoadedSensorBoundary[1].dot(feat_temp);
    }

    return return_val;
}


//*************************************************************************************************************

void BCI::clearFeatures()
{
    m_qMutex.lock();
        m_lFeaturesSensor.clear();
    m_qMutex.unlock();
}


//*************************************************************************************************************

void BCI::clearClassifications()
{
    m_qMutex.lock();
        m_lClassResultsSensor.clear();
    m_qMutex.unlock();
}


//*************************************************************************************************************

bool BCI::hasThresholdArtefact(const QList< QPair<int,RowVectorXd> > &data)
{
    // Perform simple threshold artefact reduction
    double max = 0;
    double min = 0;

    if(m_bUseArtefactThresholdReduction)
    {
        // find min max in current m_matSlidingWindowSensor/qlMatrixRows after mean was subtracted
        for(int i = 0; i<data.size(); i++)
        {
            if(data.at(i).second.maxCoeff() > max)
                max = data.at(i).second.maxCoeff();

            if(data.at(i).second.minCoeff() < min)
                min = data.at(i).second.minCoeff();
        }
    }

//    cout<<"max: "<<max<<endl;
//    cout<<"min: "<<min<<endl;

    if(max<m_dThresholdValue*1e-06 && min>m_dThresholdValue*-1e-06) // If max is outside the threshold -> completley discard m_matSlidingWindowSensor
        return false;
    else
    {
        //cout<<"Rejected artefact"<<endl;
        return true;
    }
}


//*************************************************************************************************************

bool BCI::lookForTrigger(const MatrixXd &data)
{
    // Check if capacitive touch trigger signal was received - Note that there can also be "beep" triggers in the received data, which are only 1 sample wide -> therefore look for 2 samples with a value of 254 each
    for(int i = 0; i<data.cols()-1; i++)
    {
        if(data(0,i) == 254 && data(0,i+1) == 254) // data - corresponds with channel 136 which is the trigger channel
            return true;
    }

    return false;
}


//*************************************************************************************************************

void BCI::run()
{
    while(m_bIsRunning)
    {
        // Decide which data to use - sensor or source level data
        if(m_bUseSensorData)
            BCIOnSensorLevel();
        else
            if(m_bUseSourceData)
                BCIOnSourceLevel();
    }
}


//*************************************************************************************************************

void BCI::BCIOnSensorLevel()
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
            //cout<<"About to pop matrix"<<endl;
            MatrixXd t_mat = m_pBCIBuffer_Sensor->pop();
            //cout<<"poped matrix"<<endl;

            // Get only the rows from the matrix which correspond with the selected features, namely electrodes on sensor level and destrieux clustered regions on source level
            for(int i = 0; i < m_matSlidingWindowSensor.rows(); i++)
                m_matSlidingWindowSensor.block(i, m_iTBWIndexSensor, 1, t_mat.cols()) = t_mat.block(m_mapElectrodePinningScheme[m_slChosenFeatureSensor.at(i)], 0, 1, t_mat.cols());

            m_matStimChannelSensor.block(0, m_iTBWIndexSensor, 1, t_mat.cols()) = t_mat.block(136, 0, 1, t_mat.cols());

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
            //cout<<"About to pop matrix"<<endl;
            MatrixXd t_mat = m_pBCIBuffer_Sensor->pop();
            //cout<<"poped matrix"<<endl;

            // Get only the rows from the matrix which correspond with the selected features, namely electrodes on sensor level and destrieux clustered regions on source level
            for(int i = 0; i < m_matTimeBetweenWindowsSensor.rows(); i++)
                m_matTimeBetweenWindowsSensor.block(i, m_iTBWIndexSensor, 1, t_mat.cols()) = t_mat.block(m_mapElectrodePinningScheme[m_slChosenFeatureSensor.at(i)], 0, 1, t_mat.cols());

            m_matTimeBetweenWindowsStimSensor.block(0, m_iTBWIndexSensor, 1, t_mat.cols()) = t_mat.block(136, 0, 1, t_mat.cols());

            m_iTBWIndexSensor = m_iTBWIndexSensor + t_mat.cols();
        }
        else // Recalculate m_matSlidingWindowSensor -> Calculate features, classify and store results
        {
            // ----1---- Recalculate m_matSlidingWindowSensor
            //cout<<"----1----"<<endl;
            //Move block from the right to the left -> use eval() to resolve Eigens aliasing problem
            m_matSlidingWindowSensor.block(0, 0, m_matSlidingWindowSensor.rows(), m_matSlidingWindowSensor.cols()-m_matTimeBetweenWindowsSensor.cols()) = m_matSlidingWindowSensor.block(0, m_matTimeBetweenWindowsSensor.cols(), m_matSlidingWindowSensor.rows(), m_matSlidingWindowSensor.cols()-m_matTimeBetweenWindowsSensor.cols()).eval();
            m_matStimChannelSensor.block(0, 0, m_matStimChannelSensor.rows(), m_matStimChannelSensor.cols()-m_matTimeBetweenWindowsStimSensor.cols()) = m_matStimChannelSensor.block(0, m_matTimeBetweenWindowsStimSensor.cols(), m_matStimChannelSensor.rows(), m_matStimChannelSensor.cols()-m_matTimeBetweenWindowsStimSensor.cols()).eval();

            // push m_matTimeBetweenWindowsSensor from the right
            m_matSlidingWindowSensor.block(0, m_matSlidingWindowSensor.cols()-m_matTimeBetweenWindowsSensor.cols(), m_matTimeBetweenWindowsSensor.rows(), m_matTimeBetweenWindowsSensor.cols()) = m_matTimeBetweenWindowsSensor;
            m_matStimChannelSensor.block(0, m_matStimChannelSensor.cols()-m_matTimeBetweenWindowsStimSensor.cols(), m_matTimeBetweenWindowsStimSensor.rows(), m_matTimeBetweenWindowsStimSensor.cols()) = m_matTimeBetweenWindowsStimSensor;

            //cout<<m_matStimChannelSensor;

            // Test if data is correctly streamed to this plugin
            if(m_slChosenFeatureSensor.contains("TEST"))
            {
                cout<<"Recalculate matrix"<<endl;

                for(int i = 0; i<m_matSlidingWindowSensor.cols() ; i++)
                    cout << m_matSlidingWindowSensor(m_matSlidingWindowSensor.rows()-1,i) <<endl;

//                    for(int i = 1; i<m_matSlidingWindowSensor.cols() ; i++)
//                    {
//                        cout << m_matSlidingWindowSensor(m_matSlidingWindowSensor.rows()-1,i-1) <<endl;
//                        if(m_matSlidingWindowSensor(m_matSlidingWindowSensor.rows()-1,i) - m_matSlidingWindowSensor(m_matSlidingWindowSensor.rows()-1,i-1) != 1)
//                            cout<<"Sequence error while streaming from tmsi plugin at position: "<<i<<endl;
//                    }
            }

            // ----2---- Transform matrix into QList structure, so that QTConcurrent can handle it properly
            //cout<<"----2----"<<endl;
            QList< QPair<int,RowVectorXd> > qlMatrixRows;
            for(int i = 0; i< m_matSlidingWindowSensor.rows(); i++)
                qlMatrixRows << QPair<int,RowVectorXd>(i, m_matSlidingWindowSensor.row(i));

            int iNumberOfFeatures = qlMatrixRows.size();

            // ----3---- Subtract mean in m_matSlidingWindowSensor concurrently using map()
            //cout<<"----3----"<<endl;
            if(m_bSubtractMean)
            {
                QFuture<void> futureMean = QtConcurrent::map(qlMatrixRows,[this](QPair<int,RowVectorXd>& rowdata) {
                    applyMeanCorrectionConcurrently(rowdata);
                });

                futureMean.waitForFinished();
            }

            // ----4---- Do simple threshold artefact reduction
            //cout<<"----4----"<<endl;
            if(hasThresholdArtefact(qlMatrixRows) == false)
            {
                // Look for trigger flag
                if(lookForTrigger(m_matStimChannelSensor) && !m_bTriggerActivated)
                {
                    // cout << "Trigger activated" << endl;
                    //QFuture<void> future = QtConcurrent::run(Beep, 450, 700);
                    m_bTriggerActivated = true;
                }

                // ----5---- Filter data in m_matSlidingWindowSensor concurrently using map()
                //cout<<"----5----"<<endl;
                // TODO: work only on qlMatrixRows -> filteredRows doesnt need to be created -> more efficient
                QList< QPair<int,RowVectorXd> > filteredRows = qlMatrixRows;

                if(m_bUseFilter)
                {
                    QFuture<void> futureFilter = QtConcurrent::map(filteredRows,[this](QPair<int,RowVectorXd>& chdata) {
                        applyFilterOperatorConcurrently(chdata);
                    });

                    futureFilter.waitForFinished();
                }

//                    // Write data before and after filtering to debug file
//                    for(int i=0; i<qlMatrixRows.size(); i++)
//                        m_outStreamDebug << "qlMatrixRows at row " << i << ": " << qlMatrixRows.at(i).second << "\n";

//                    m_outStreamDebug<<endl;

//                    for(int i=0; i<filteredRows.size(); i++)
//                        m_outStreamDebug << "filteredRows at row " << i << ": " << filteredRows.at(i).second << "\n";

//                    m_outStreamDebug << endl << "---------------------------------------------------------" << endl << endl;

//                    // Write filtered data continously to file
//                    for(int i = 0; i<filteredRows.at(0).second.size() ;i++)
//                        m_outStreamDebug << filteredRows.at(0).second(i)<<endl;

                // ----6---- Calculate features concurrently using mapped()
                //cout<<"----6----"<<endl;
                std::function< QPair<int,QList<double> > (QPair<int,RowVectorXd>&)> applyOpsFeatures = [this](QPair<int,RowVectorXd>& chdata) -> QPair< int,QList<double> > {
                    return applyFeatureCalcConcurrentlyOnSensorLevel(chdata);
                };

                QFuture< QPair< int,QList<double> > > futureCalculatedFeatures = QtConcurrent::mapped(filteredRows.begin(), filteredRows.end(), applyOpsFeatures);

                futureCalculatedFeatures.waitForFinished();

                m_iNumberOfCalculatedFeatures++;

                // ----7---- Store features
                //cout<<"----7----"<<endl;
                m_lFeaturesSensor.append(futureCalculatedFeatures.results());

                // ----8---- If enough features (windows) have been calculated (processed) -> classify all features and average results
                //cout<<"----8----"<<endl;
                if(m_iNumberOfCalculatedFeatures == m_iNumberFeatures)
                {
                    // Transform m_lFeaturesSensor into an easier file structure -> create feature points
                    QList< QList<double> > lFeaturesSensor_new;

                    for(int i = 0; i<m_lFeaturesSensor.size()-iNumberOfFeatures+1; i = i + iNumberOfFeatures) // iterate over QPair feature List
                        for(int z = 0; z<m_lFeaturesSensor.at(0).second.size(); z++) // iterate over number of sub signals
                        {
                            QList<double> temp;
                            for(int t = 0; t<iNumberOfFeatures; t++) // iterate over chosen features (electrodes)
                                temp.append(m_lFeaturesSensor.at(i+t).second.at(z));
                            lFeaturesSensor_new.append(temp);
                        }

//                        //Check sizes
//                        cout<<"lFeaturesSensor_new.size()"<<lFeaturesSensor_new.size()<<endl;
//                        cout<<"lFeaturesSensor_new.at(0).size()"<<lFeaturesSensor_new.at(0).size()<<endl;
//                        cout<<"m_lFeaturesSensor.size()"<<m_lFeaturesSensor.size()<<endl;

                    // Display features
                    if(m_bDisplayFeatures)
                        emit paintFeatures((MyQList)lFeaturesSensor_new, m_bTriggerActivated);

                    // Reset trigger
                    m_bTriggerActivated = false;

                    // ----9---- Classify features concurrently using mapped() ----------
                    //cout<<"----9----"<<endl;

                    std::function<double (QList<double>&)> applyOpsClassification = [this](QList<double>& featData){
                        return applyClassificationCalcConcurrentlyOnSensorLevel(featData);
                    };

                    QFuture<double> futureClassificationResults = QtConcurrent::mapped(lFeaturesSensor_new.begin(), lFeaturesSensor_new.end(), applyOpsClassification);

                    futureClassificationResults.waitForFinished();

                    // ----10---- Generate final classification result -> average all classification results
                    //cout<<"----10----"<<endl;
                    double dfinalResult = 0;

                    for(int i = 0; i<futureClassificationResults.resultCount() ;i++)
                        dfinalResult += futureClassificationResults.resultAt(i);

                    dfinalResult = dfinalResult/futureClassificationResults.resultCount();
                    cout << "dfinalResult: " << dfinalResult << endl << endl;

                    // ----11---- Store final result
                    //cout<<"----11----"<<endl;
                    m_lClassResultsSensor.append(dfinalResult);

                    // ----12---- Send result to the output stream, i.e. which is connected to the triggerbox
                    //cout<<"----12----"<<endl;
                    VectorXd variances(iNumberOfFeatures);
                    variances.setZero();

                    for(int i = 0; i<lFeaturesSensor_new.size(); i++)
                        for(int t = 0; t<iNumberOfFeatures; t++)
                            variances(t) = variances(t) + lFeaturesSensor_new.at(i).at(t);

                    variances = variances/lFeaturesSensor_new.size();

                    m_pBCIOutputOne->data()->setValue(dfinalResult);
                    m_pBCIOutputTwo->data()->setValue(variances(0));
                    m_pBCIOutputThree->data()->setValue(variances(1));

                    for(int i = 0; i<filteredRows.at(0).second.cols() ; i++)
                    {
                        m_pBCIOutputFour->data()->setValue(filteredRows.at(0).second(0,i));
                        m_pBCIOutputFive->data()->setValue(filteredRows.at(1).second(0,i));
                    }

                    // Clear classifications
                    clearFeatures();

                    // Reset counter
                    m_iNumberOfCalculatedFeatures = 0;
                } // End if enough features (windows) have been calculated (processed)
            } // End if artefact reduction
            else
            {
                // If trial has been rejected -> plot zeros as result and the filtered electrode channel
                m_pBCIOutputOne->data()->setValue(0);
                m_pBCIOutputTwo->data()->setValue(0);
                m_pBCIOutputThree->data()->setValue(0);

                QList< QPair<int,RowVectorXd> > filteredRows = qlMatrixRows;

                if(m_bUseFilter)
                {
                    QFuture<void> futureFilter = QtConcurrent::map(filteredRows,[this](QPair<int,RowVectorXd>& chdata) {
                        applyFilterOperatorConcurrently(chdata);
                    });

                    futureFilter.waitForFinished();
                }

                for(int i = 0; i<filteredRows.at(0).second.cols() ; i++)
                {
                    m_pBCIOutputFour->data()->setValue(filteredRows.at(0).second(0,i));
                    m_pBCIOutputFive->data()->setValue(filteredRows.at(1).second(0,i));
                }
            }

            m_iTBWIndexSensor = 0;
        }
    }
}


//*************************************************************************************************************

void BCI::BCIOnSourceLevel()
{
}
