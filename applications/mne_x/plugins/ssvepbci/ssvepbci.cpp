//=============================================================================================================
/**
* @file     ssvepbci.cpp
* @author   Viktor Klüber <viktor.klueber@tu-ilmenau.de>;
*           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     Day, 2016
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

#include "ssvepbci.h"


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

using namespace ssvepBCIPlugin;
using namespace std;
using namespace UTILSLIB;

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ssvepBCI::ssvepBCI()
: m_qStringResourcePath(qApp->applicationDirPath()+"/mne_x_plugins/resources/ssvepbci/")
, m_bProcessData(false)
{
}


//*************************************************************************************************************

ssvepBCI::~ssvepBCI()
{
    //std::cout << "BCI::~BCI() " << std::endl;

    //If the program is closed while the sampling is in process
    if(this->isRunning())
        this->stop();
}

//*************************************************************************************************************

//QSharedPointer<IPlugin> ssvepBCI::clone() const
//{
//    QSharedPointer<ssvepBCI> pTMSIClone(new ssvepBCI());
//    return pTMSIClone;
//}

//*************************************************************************************************************

QSharedPointer<IPlugin> ssvepBCI::clone() const
{
    QSharedPointer<ssvepBCI> pTMSIClone(new ssvepBCI());
    return pTMSIClone;
}


//*************************************************************************************************************

void ssvepBCI::init()
{
    m_bIsRunning = false;
    m_bTriggerActivated = false;

//    // Inputs - Source estimates and sensor level
//    m_pRTSEInput = PluginInputData<RealTimeSourceEstimate>::create(this, "BCIInSource", "BCI source input data");
//    connect(m_pRTSEInput.data(), &PluginInputConnector::notify, this, &BCI::updateSource, Qt::DirectConnection);
//    m_inputConnectors.append(m_pRTSEInput);

//    m_pRTMSAInput = PluginInputData<NewRealTimeMultiSampleArray>::create(this, "BCIInSensor", "SourceLab sensor input data");
//    connect(m_pRTMSAInput.data(), &PluginInputConnector::notify, this, &BCI::updateSensor, Qt::DirectConnection);
//    m_inputConnectors.append(m_pRTMSAInput);

//    // Output streams
//    m_pBCIOutputOne = PluginOutputData<NewRealTimeSampleArray>::create(this, "ControlSignal", "BCI output data One");
//    m_pBCIOutputOne->data()->setArraySize(1);
//    m_pBCIOutputOne->data()->setName("Boundary");
//    m_outputConnectors.append(m_pBCIOutputOne);

//    m_pBCIOutputTwo = PluginOutputData<NewRealTimeSampleArray>::create(this, "ControlSignal", "BCI output data Two");
//    m_pBCIOutputTwo->data()->setArraySize(1);
//    m_pBCIOutputTwo->data()->setName("Left electrode var");
//    m_outputConnectors.append(m_pBCIOutputTwo);

//    m_pBCIOutputThree = PluginOutputData<NewRealTimeSampleArray>::create(this, "ControlSignal", "BCI output data Three");
//    m_pBCIOutputThree->data()->setArraySize(1);
//    m_pBCIOutputThree->data()->setName("Right electrode var");
//    m_outputConnectors.append(m_pBCIOutputThree);

//    m_pBCIOutputFour = PluginOutputData<NewRealTimeSampleArray>::create(this, "ControlSignal", "BCI output data Four");
//    m_pBCIOutputFour->data()->setArraySize(1);
//    m_pBCIOutputFour->data()->setName("Left electrode");
//    m_outputConnectors.append(m_pBCIOutputFour);

//    m_pBCIOutputFive = PluginOutputData<NewRealTimeSampleArray>::create(this, "ControlSignal", "BCI output data Five");
//    m_pBCIOutputFive->data()->setArraySize(1);
//    m_pBCIOutputFive->data()->setName("Right electrode");
//    m_outputConnectors.append(m_pBCIOutputFive);

//    // Delete Buffer - will be initailzed with first incoming data
//    m_pBCIBuffer_Sensor = CircularMatrixBuffer<double>::SPtr();
//    m_pBCIBuffer_Source = CircularMatrixBuffer<double>::SPtr();

//    // Delete fiff info because the initialisation of the fiff info is seen as the first data acquisition from the input stream
//    m_pFiffInfo_Sensor = FiffInfo::SPtr();

//    // Intitalise GUI stuff
//    m_bSubtractMean = true;
//    m_bUseFilter = true;
//    m_bUseSensorData = true;
//    m_bUseSourceData = false;
//    m_bDisplayFeatures = true;
//    m_bUseArtefactThresholdReduction = false;
//    m_dSlidingWindowSize = 0.5;
//    m_dTimeBetweenWindows = 0.5;
//    m_dDisplayRangeBoundary = 15;
//    m_dDisplayRangeVariances = 5;
//    m_dDisplayRangeElectrodes = 7;
//    m_iNumberFeatures = 1;
//    m_dThresholdValue = 30;
//    m_iNumberFeaturesToDisplay = 30;
//    m_iFeatureCalculationType = 0;

//    // Intitalise feature selection
//    m_slChosenFeatureSensor << "LA4" << "RA4"; //<< "TEST";

//    // Initalise sliding window stuff
//    m_bFillSensorWindowFirstTime = true;

//    // Initialise filter stuff
//    m_filterOperator = QSharedPointer<FilterData>(new FilterData());

//    m_dFilterLowerBound = 7.0;
//    m_dFilterUpperBound = 14.0;
//    m_dParcksWidth = m_dFilterLowerBound-1; // (m_dFilterUpperBound-m_dFilterLowerBound)/2;
//    m_iFilterOrder = 256;

//    // Init BCIFeatureWindow for visualization
//    m_BCIFeatureWindow = QSharedPointer<BCIFeatureWindow>(new BCIFeatureWindow(this));
}


//*************************************************************************************************************

void ssvepBCI::unload()
{

}


//*************************************************************************************************************

bool ssvepBCI::start()
{
//    // Do not start BCI if the number of chosen electrodes/features is uneven
//    if(m_slChosenFeatureSensor.size()%2 != 0)
//    {
//        if(!m_slChosenFeatureSensor.contains("TEST"))
//        {
//            QMessageBox msgBox;
//            msgBox.setText("The number of selected electrodes needs to be even.");
//            msgBox.setInformativeText("Please select feautres again");
//            msgBox.setStandardButtons(QMessageBox::Ok);
//            msgBox.exec();
//            return false;
//        }
//    }

//    // Initialise index
//    m_iTBWIndexSensor = 0;
//    m_iNumberOfCalculatedFeatures = 0;

//    // BCIFeatureWindow show and init
//    if(m_bDisplayFeatures)
//    {
//        m_BCIFeatureWindow->initGui();
//        m_BCIFeatureWindow->show();
//    }

//    // Init debug output stream
//    QString path("BCIDebugFile.txt");
//    path.prepend(m_qStringResourcePath);
//    m_outStreamDebug.open(path.toStdString(), ios::trunc);

//    m_pFiffInfo_Sensor = FiffInfo::SPtr();

//    m_bFillSensorWindowFirstTime = true;

////    // Set display ranges for output channels
////    m_pBCIOutputOne->data()->setMaxValue(m_dDisplayRangeBoundary);
////    m_pBCIOutputOne->data()->setMinValue(-m_dDisplayRangeBoundary);

////    m_pBCIOutputTwo->data()->setMaxValue(m_dDisplayRangeVariances);
////    m_pBCIOutputTwo->data()->setMinValue(0);

////    m_pBCIOutputThree->data()->setMaxValue(m_dDisplayRangeVariances);
////    m_pBCIOutputThree->data()->setMinValue(0);

////    m_pBCIOutputFour->data()->setMaxValue(m_dDisplayRangeElectrodes);
////    m_pBCIOutputFour->data()->setMinValue(-m_dDisplayRangeElectrodes);

////    m_pBCIOutputFive->data()->setMaxValue(m_dDisplayRangeElectrodes);
////    m_pBCIOutputFive->data()->setMinValue(-m_dDisplayRangeElectrodes);

//    // variance
//    m_pBCIOutputOne->data()->setMaxValue(5);
//    m_pBCIOutputOne->data()->setMinValue(-5);

//    m_pBCIOutputTwo->data()->setMaxValue(10);
//    m_pBCIOutputTwo->data()->setMinValue(0);

//    m_pBCIOutputThree->data()->setMaxValue(10);
//    m_pBCIOutputThree->data()->setMinValue(0);

//    m_pBCIOutputFour->data()->setMaxValue(1e-05);
//    m_pBCIOutputFour->data()->setMinValue(-1e-05);

//    m_pBCIOutputFive->data()->setMaxValue(1e-05);
//    m_pBCIOutputFive->data()->setMinValue(-1e-05);

////    // log variance
////    m_pBCIOutputOne->data()->setMaxValue(10);
////    m_pBCIOutputOne->data()->setMinValue(-10);

////    m_pBCIOutputTwo->data()->setMaxValue(15);
////    m_pBCIOutputTwo->data()->setMinValue(0);

////    m_pBCIOutputThree->data()->setMaxValue(15);
////    m_pBCIOutputThree->data()->setMinValue(0);

////    m_pBCIOutputFour->data()->setMaxValue(10e-04);
////    m_pBCIOutputFour->data()->setMinValue(-10e-04);

////    m_pBCIOutputFive->data()->setMaxValue(10e-04);
////    m_pBCIOutputFive->data()->setMinValue(-10e-04);

//    m_bIsRunning = true;

//    QThread::start();

    return true;
}


//*************************************************************************************************************

bool ssvepBCI::stop()
{
    m_bIsRunning = false;

//    // Get data buffers out of idle state if they froze in the acquire or release function
//    //In case the semaphore blocks the thread -> Release the QSemaphore and let it exit from the pop function (acquire statement)

//    if(m_bProcessData) // Only clear if buffers have been initialised
//    {
//        m_pBCIBuffer_Sensor->releaseFromPop();
//        m_pBCIBuffer_Sensor->releaseFromPush();
//        //    m_pBCIBuffer_Source->releaseFromPop();
//        //    m_pBCIBuffer_Source->releaseFromPush();
//    }

//    // Stop filling buffers with data from the inputs
//    m_bProcessData = false;

//    // Reset trigger
//    m_bTriggerActivated = false;

//    // Clear stream
//    m_outStreamDebug.close();
//    m_outStreamDebug.clear();

//    // Hide feature visualization window
//    if(m_bDisplayFeatures)
//        m_BCIFeatureWindow->hide();

//    // Delete all features and classification results
//    clearFeatures();
//    clearClassifications();

    return true;
}


//*************************************************************************************************************

IPlugin::PluginType ssvepBCI::getType() const
{
    return _IAlgorithm;
}


//*************************************************************************************************************

QString ssvepBCI::getName() const
{
    return "SSVEP BCI EEG";
}


//*************************************************************************************************************

QWidget* ssvepBCI::setupWidget()
{
    ssvepBCISetupWidget* setupWidget = new ssvepBCISetupWidget(this);//widget is later destroyed by CentralWidget - so it has to be created everytime new

    //init properties dialog
    setupWidget->initGui();

    return setupWidget;
}


//*************************************************************************************************************

void ssvepBCI::updateSensor(XMEASLIB::NewMeasurement::SPtr pMeasurement)
{
//    QSharedPointer<NewRealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<NewRealTimeMultiSampleArray>();
//    if(pRTMSA)
//    {
//        //Check if buffer initialized
//        if(!m_pBCIBuffer_Sensor)
//            m_pBCIBuffer_Sensor = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(64, pRTMSA->getNumChannels(), pRTMSA->getMultiArraySize()));

//        // Load Fiff information on sensor level
//        if(!m_pFiffInfo_Sensor)
//        {
//            m_pFiffInfo_Sensor = pRTMSA->getFiffInfo();

//            // Adjust working matrixes (sliding window and time between windows matrix) size so that the samples from the tmsi plugin stream fit in the matrix perfectly
//            int arraySize = pRTMSA->getMultiArraySize();
//            int modulo = int(m_pFiffInfo_Sensor->sfreq*m_dSlidingWindowSize) % arraySize;
//            int rows = m_slChosenFeatureSensor.size();
//            int cols = m_pFiffInfo_Sensor->sfreq*m_dSlidingWindowSize-modulo;
//            m_matSlidingWindowSensor.resize(rows, cols);

//            m_matStimChannelSensor.resize(1,cols);

//            modulo = int(m_pFiffInfo_Sensor->sfreq*m_dTimeBetweenWindows) % arraySize;
//            rows = m_slChosenFeatureSensor.size();
//            cols = m_pFiffInfo_Sensor->sfreq*m_dTimeBetweenWindows-modulo;
//            m_matTimeBetweenWindowsSensor.resize(rows, cols);

//            m_matTimeBetweenWindowsStimSensor.resize(1,cols);

//            // Build filter operator
//            double dCenterFreqNyq = (m_dFilterLowerBound+((m_dFilterUpperBound - m_dFilterLowerBound)/2))/(m_pFiffInfo_Sensor->sfreq/2);
//            double dBandwidthNyq = (m_dFilterUpperBound - m_dFilterLowerBound)/(m_pFiffInfo_Sensor->sfreq/2);
//            double dParksWidth = m_dParcksWidth/(m_pFiffInfo_Sensor->sfreq/2);

////            // Calculate needed fft length
////            int exponent = ceil(log10(m_matSlidingWindowSensor.cols())/log10(2));
////            int fftLength = pow(2,exponent+1);

//            // Initialise filter operator
//            m_filterOperator = QSharedPointer<FilterData>(new FilterData(QString("BPF"),FilterData::BPF,m_iFilterOrder,dCenterFreqNyq,dBandwidthNyq,dParksWidth,m_matSlidingWindowSensor.cols()+m_iFilterOrder)); // letztes Argument muss 2er potenz sein - fft länge

//            // Write filter coefficients to debug file
//            for(int i = 0; i<m_filterOperator->m_dCoeffA.cols(); i++)
//                m_outStreamDebug << m_filterOperator->m_dFFTCoeffA(0,i).real() <<"+" << m_filterOperator->m_dFFTCoeffA(0,i).imag() << "i "  << endl;

//            m_outStreamDebug << endl << endl;

//            for(int i = 0; i<m_filterOperator->m_dCoeffA.cols(); i++)
//                m_outStreamDebug << m_filterOperator->m_dCoeffA(0,i) << endl;

//            m_outStreamDebug << "---------------------------------------------------------------------" << endl;
//        }

//        // Only process data when fiff info has been initialised in run() method
//        if(m_bProcessData)
//        {
//            MatrixXd t_mat(pRTMSA->getNumChannels(), pRTMSA->getMultiArraySize());

//            for(unsigned char i = 0; i < pRTMSA->getMultiArraySize(); ++i)
//                t_mat.col(i) = pRTMSA->getMultiSampleArray()[i];

//            m_pBCIBuffer_Sensor->push(&t_mat);
//        }
//    }
}


//*************************************************************************************************************

void ssvepBCI::updateSource(XMEASLIB::NewMeasurement::SPtr pMeasurement)
{
//    cout<<"update source"<<endl;
//    QSharedPointer<RealTimeSourceEstimate> pRTSE = pMeasurement.dynamicCast<RealTimeSourceEstimate>();
//    if(pRTSE)
//    {
//        //Check if buffer initialized
//        if(!m_pBCIBuffer_Source)
//            m_pBCIBuffer_Source = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(64, pRTSE->getValue().size(), pRTSE->getArraySize()));

//        if(m_bProcessData)
//        {
//            MatrixXd t_mat(pRTSE->getValue().size(), pRTSE->getArraySize());

//            for(unsigned char i = 0; i < pRTSE->getArraySize(); ++i)
//                t_mat.col(i) = pRTSE->getStc().data.col(i);

//            m_pBCIBuffer_Source->push(&t_mat);
//        }
//    }
}

//*************************************************************************************************************

void ssvepBCI::clearFeatures()
{
    m_qMutex.lock();
        m_lFeaturesSensor.clear();
    m_qMutex.unlock();
}


//*************************************************************************************************************

void ssvepBCI::clearClassifications()
{
    m_qMutex.lock();
        m_lClassResultsSensor.clear();
    m_qMutex.unlock();
}

//*************************************************************************************************************

bool ssvepBCI::lookForTrigger(const MatrixXd &data)
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

void ssvepBCI::run()
{
    while(m_bIsRunning)
    {
        // Decide which data to use - sensor or source level data
        if(m_bUseSensorData)
            BCIOnSensorLevel();
        else
            ;
//            if(m_bUseSourceData)
                //BCIOnSourceLevel();
    }
}


//*************************************************************************************************************

void ssvepBCI::BCIOnSensorLevel()
{

}



