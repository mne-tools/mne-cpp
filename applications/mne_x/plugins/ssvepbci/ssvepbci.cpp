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
//, m_qFile("out.txt")
//, m_sOut(&m_qFile)
{
    // Create start Stimuli action bar item/button
    m_pActionSetupStimulus = new QAction(QIcon(":/images/stimulus.png"),tr("setup stimulus feature"),this);
    m_pActionSetupStimulus->setStatusTip(tr("Setup stimulus feature"));
    connect(m_pActionSetupStimulus, &QAction::triggered, this, &ssvepBCI::showSetupStimulus);
    addPluginAction(m_pActionSetupStimulus);

    // Intitalise BCI data
    m_slChosenFeatureSensor << "9Z" << "8Z" << "7Z" << "6Z" << "9L" << "8L" << "9R" << "8R"; //<< "TEST";

    m_lElectrodeNumbers << 33 << 34 << 35 << 36 << 40 << 41 << 42 << 43;
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

    // Inputs - Source estimates and sensor level
    m_pRTSEInput = PluginInputData<RealTimeSourceEstimate>::create(this, "BCIInSource", "BCI source input data");
    connect(m_pRTSEInput.data(), &PluginInputConnector::notify, this, &ssvepBCI::updateSource, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTSEInput);

    m_pRTMSAInput = PluginInputData<NewRealTimeMultiSampleArray>::create(this, "BCIInSensor", "SourceLab sensor input data");
    connect(m_pRTMSAInput.data(), &PluginInputConnector::notify, this, &ssvepBCI::updateSensor, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTMSAInput);

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

    // Delete Buffer - will be initailzed with first incoming data
    m_pBCIBuffer_Sensor = CircularMatrixBuffer<double>::SPtr();
    m_pBCIBuffer_Source = CircularMatrixBuffer<double>::SPtr();

    // Delete fiff info because the initialisation of the fiff info is seen as the first data acquisition from the input stream
    m_pFiffInfo_Sensor = FiffInfo::SPtr();

    // Intitalise GUI stuff
    m_bUseSensorData = true;

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

    // Init debug output stream
    QString path("BCIDebugFile.txt");
    path.prepend(m_qStringResourcePath);
    m_outStreamDebug.open(path.toStdString(), ios::trunc);

    m_pFiffInfo_Sensor = FiffInfo::SPtr();

    m_iWriteIndex   = 0;
    m_iReadIndex    = 0;
    m_iCounter      = 0;
    m_iReadToWriteBuffer = 0;
    m_iWindowSize        = 8;
    m_bIsRunning    = true;

    QThread::start();

//    if (!m_qFile.open(QIODevice::WriteOnly | QIODevice::Text))

    return true;
}


//*************************************************************************************************************

bool ssvepBCI::stop()
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

//    // Reset trigger
//    m_bTriggerActivated = false;

    // Clear stream
    m_outStreamDebug.close();
    m_outStreamDebug.clear();

//    // Hide feature visualization window
//    if(m_bDisplayFeatures)
//        m_BCIFeatureWindow->hide();

    // Delete all features and classification results
    clearFeatures();
    clearClassifications();

    // set counters to zero
    m_iWriteIndex   = 0;
    m_iReadIndex    = 0;
    m_iCounter      = 0;
    m_iReadToWriteBuffer = 0;

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


    QSharedPointer<NewRealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<NewRealTimeMultiSampleArray>();
    if(pRTMSA)
    {
        //Check if buffer initialized

        m_qMutex.lock();
        if(!m_pBCIBuffer_Sensor)
            m_pBCIBuffer_Sensor = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(64, pRTMSA->getNumChannels(), pRTMSA->getMultiSampleArray()[0].cols()));
    }

    //Fiff information
    if(!m_pFiffInfo_Sensor)
    {
        m_pFiffInfo_Sensor = pRTMSA->info();
        //emit fiffInfoAvailable();

        //loading fiff information
        m_iSampleFrequency = m_pFiffInfo_Sensor->sfreq;

        // determine sliding time window parameters
        int rows = 8;
        m_iReadSampleSize = 0.1*m_iSampleFrequency;    // about 0.1 second long time segment as basic read increment
        m_iWriteSampleSize = pRTMSA->getMultiSampleArray()[0].cols();
        m_iTimeWindowSegmentSize  = int(4*m_iSampleFrequency / m_iWriteSampleSize) + 1;   // 4 seconds long maximal sized window
        m_matSlidingTimeWindow.resize(rows, m_iTimeWindowSegmentSize*pRTMSA->getMultiSampleArray()[0].cols());
    }
    m_qMutex.unlock();




    if(m_bProcessData)
    {
        MatrixXd t_mat;

        for(qint32 i = 0; i < pRTMSA->getMultiArraySize(); ++i)
        {
            t_mat = pRTMSA->getMultiSampleArray()[i];
            m_pBCIBuffer_Sensor->push(&t_mat);
        }
    }

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

void ssvepBCI::showSetupStimulus()
{
    QDesktopWidget Desktop; // Desktop Widget for getting the number of accessible screens

    if(Desktop.numScreens()> 1){
        // Open setup stimulus widget
        if(m_pssvepBCISetupStimulusWidget == NULL)
            m_pssvepBCISetupStimulusWidget = QSharedPointer<ssvepBCISetupStimulusWidget>(new ssvepBCISetupStimulusWidget(this));

        if(!m_pssvepBCISetupStimulusWidget->isVisible()){

            m_pssvepBCISetupStimulusWidget->setWindowTitle("ssvepBCI - Setup Stimulus");
            //m_pEEGoSportsSetupStimulusWidget->initGui();
            m_pssvepBCISetupStimulusWidget->show();
            m_pssvepBCISetupStimulusWidget->raise();

        }
    }
    else{
        QMessageBox msgBox;
        msgBox.setText("Only one screen detected! For stimulus visualization attach one more.");
        msgBox.exec();
        return;
    }
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

void ssvepBCI::readFromSlidingTimeWindow(MatrixXd &data)
{
    // consider matrix overflow case
    if(data.cols() > m_iReadIndex){
        int width1 = data.cols() - m_iReadIndex;
        data.block(0, 0, 8, width1) = m_matSlidingTimeWindow.block(0, m_matSlidingTimeWindow.cols() - 1 - width1, 8, width1 );
        data.block(0, width1, 8, m_iReadIndex) = m_matSlidingTimeWindow.block(0, 0, 8, m_iReadIndex);
    }
    else
        data = m_matSlidingTimeWindow.block(0, m_iReadIndex - data.cols(), 8 , data.cols());

}


//*************************************************************************************************************

void ssvepBCI::BCIOnSensorLevel()
{
    // Wait for fiff Info if not yet received - this is needed because we have to wait until the buffers are firstly initiated in the update functions
    while(!m_pFiffInfo_Sensor)
        msleep(10);

    // Start filling buffers with data from the inputs
    m_bProcessData = true;
    MatrixXd t_mat = m_pBCIBuffer_Sensor->pop();

    // writing selected feature channels to the time window storage and increase the segment index
    for(int i = 0; i < 8; i++)
        m_matSlidingTimeWindow.block(i, m_iWriteIndex, 1, m_iWriteSampleSize) = t_mat.block(m_lElectrodeNumbers.at(i), 0, 1, m_iWriteSampleSize);
    m_iWriteIndex = (m_iWriteIndex + m_iWriteSampleSize) % (m_iWriteSampleSize*m_iTimeWindowSegmentSize);

    // calculate buffer between read- and write index
    m_iReadToWriteBuffer = m_iReadToWriteBuffer + m_iWriteSampleSize;
    cout << "Read to Write buffer:" << m_iReadToWriteBuffer << endl;
    // execute processing loop as long as there is new data to read from the time window
    while(m_iReadToWriteBuffer >= m_iReadSampleSize)
    {
        if(m_iCounter > 8)
        {
            // determine window size according to former success of former classifications
            if(m_iCounter <= 24)
                m_iWindowSize = 8;
            if(m_iCounter <= 44 && m_iCounter > 24)
                m_iWindowSize = 20;
            if(m_iCounter > 44)
                m_iWindowSize = 40;

            MatrixXd Y_data;
            Y_data.resize(8, m_iWindowSize*m_iReadSampleSize);
            readFromSlidingTimeWindow(Y_data);

            //std::cout << "Matrix:\n" << m_matSlidingTimeWindow.block(0, m_iReadIndex, 8, 5) << endl;

            cout << "Window Size:" << m_iWindowSize << endl;
            cout << "Y_data rows" << Y_data.rows() << endl;
            cout << "Y_data cols" << Y_data.cols() << endl;




        }

        // update counter variables
        m_iCounter++;
        m_iReadToWriteBuffer = m_iReadToWriteBuffer - m_iReadSampleSize;
        m_iReadIndex = (m_iReadIndex + m_iReadSampleSize) % (m_iWriteSampleSize*m_iTimeWindowSegmentSize);
    }
    



    cout << "Write Sample Size:" << m_iWriteSampleSize << endl;
    cout << "Read Sample Size:" << m_iReadSampleSize << endl;
    cout << "Write Index:" << m_iWriteIndex << endl;
    cout << "Read Index:" << m_iReadIndex << endl;
    cout << "Time Window Segment Size:" << m_iTimeWindowSegmentSize << endl;
    cout << "Window Size" << m_iWriteSampleSize*m_iTimeWindowSegmentSize << endl << endl;




    // increase ringbuffer increment


}


