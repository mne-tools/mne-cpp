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

    m_pRTMSAInput = PluginInputData<NewRealTimeMultiSampleArray>::create(this, "BCIInSensor", "SourceLab sensor input data");
    connect(m_pRTMSAInput.data(), &PluginInputConnector::notify, this, &BCI::updateSensor, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTMSAInput);

    // Output
    m_pBCIOutput = PluginOutputData<NewRealTimeSampleArray>::create(this, "ControlSignal", "BCI output data");
    m_pBCIOutput->data()->setArraySize(1);
    m_pBCIOutput->data()->setMaxValue(1);
    m_pBCIOutput->data()->setMinValue(-1);
    m_outputConnectors.append(m_pBCIOutput);

    //m_pBCIOutput->data()->setMaxValue();

    //Delete Buffer - will be initailzed with first incoming data
    m_pBCIBuffer_Sensor = CircularMatrixBuffer<double>::SPtr();
    m_pBCIBuffer_Source = CircularMatrixBuffer<double>::SPtr();

    // Delete fiff info because the initialisation of the fiff info is seen as the first data acquisition from the input stream
    m_pFiffInfo_Sensor = FiffInfo::SPtr();

    // Intitalise GUI stuff
    m_bSubtractMean = true;
    m_bUseFilter = true;
    m_bUseSensorData = true;
    m_bUseSourceData = false;
    m_bUseArtefactThresholdReduction = true;
    m_dSlidingWindowSize = 1.0;
    m_dTimeBetweenWindows = 0.04;
    m_iNumberSubSignals = 1;
    m_dThresholdValue = 25;
    m_sSensorBoundaryPath = m_qStringResourcePath;
    m_sSourceBoundaryPath = m_qStringResourcePath;

    // Intitalise feature selection
    m_slChosenFeatureSensor << "LA4" << "RA4"; //<< "TEST";
    m_iNumberOfCalculatedFeatures = 0;

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
    m_iFilterOrder = 128;

    // Init BCIFeatureWindow for visualization
    m_BCIFeatureWindow = QSharedPointer<BCIFeatureWindow>(new BCIFeatureWindow(this));
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

    // BCIFeatureWindow show and init
    m_BCIFeatureWindow->initGui();
    m_BCIFeatureWindow->show();

    // Init debug output stream
    QString path("filterOutput.txt");
    path.prepend(m_qStringResourcePath);
    m_outStreamDebug.open(path.toStdString(), ios::trunc);

    m_pFiffInfo_Sensor = FiffInfo::SPtr();

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

void BCI::updateSensor(XMEASLIB::NewMeasurement::SPtr pMeasurement)
{
    QSharedPointer<NewRealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<NewRealTimeMultiSampleArray>();
    if(pRTMSA)
    {
        //Check if buffer initialized
        if(!m_pBCIBuffer_Sensor)
            m_pBCIBuffer_Sensor = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(64, pRTMSA->getNumChannels(), pRTMSA->getMultiArraySize()));

        // Load Fiff information on sensor level
        if(!m_pFiffInfo_Sensor)
        {
            m_pFiffInfo_Sensor = pRTMSA->getFiffInfo();

            // Adjust working matrixes (sliding window and time between windows matrix) size so that the samples from the tmsi plugin stream fit in the matrix perfectly
            int arraySize = pRTMSA->getMultiArraySize();
            int modulo = int(m_pFiffInfo_Sensor->sfreq*m_dSlidingWindowSize) % arraySize;
            int rows = m_slChosenFeatureSensor.size();
            int cols = m_pFiffInfo_Sensor->sfreq*m_dSlidingWindowSize-modulo;
            m_matSlidingWindowSensor.resize(rows, cols);

            modulo = int(m_pFiffInfo_Sensor->sfreq*m_dTimeBetweenWindows) % arraySize;
            rows = m_slChosenFeatureSensor.size();
            cols = m_pFiffInfo_Sensor->sfreq*m_dTimeBetweenWindows-modulo;
            m_matTimeBetweenWindowsSensor.resize(rows, cols);

            // Build filter operator
            double dCenterFreqNyq = (m_dFilterLowerBound+((m_dFilterUpperBound - m_dFilterLowerBound)/2))/(m_pFiffInfo_Sensor->sfreq/2);
            double dBandwidthNyq = (m_dFilterUpperBound - m_dFilterLowerBound)/(m_pFiffInfo_Sensor->sfreq/2);
            double dParksWidth = m_dParcksWidth/(m_pFiffInfo_Sensor->sfreq/2);

            // Calculate needed fft length
            int exponent = ceil(log10(m_matSlidingWindowSensor.cols())/log10(2));
            int fftLength = pow(2,exponent+1);

            // Initialise filter operator
            m_filterOperator = QSharedPointer<FilterData>(new FilterData(QString("BPF"),FilterData::BPF,m_iFilterOrder,dCenterFreqNyq,dBandwidthNyq,dParksWidth,fftLength)); // letztes Argument muss 2er potenz sein - fft länge

            // Write filter coefficients to debug file
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

            // Check if capacitive trigger signal was received - Note that there can also be "beep" triggers in the received data, hwich are only 1 sample wide -> therefore look for 2 samples with the value 254
            for(int i = 0; i<t_mat.cols()-1; i++)
                if(t_mat(t_mat.rows()-2,i) == 254 && t_mat(t_mat.rows()-2,i+1) == 254 && m_bTriggerActivated == false) // t_mat(t_mat.rows()-2,i) - corresponds with channel 136 which is the trigger channel
                    m_bTriggerActivated = true;
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

QPair<int,QList<double>> BCI::applyFeatureCalcConcurrentlyOnSensorLevel(const QPair<int, RowVectorXd> &chdata)
{
    RowVectorXd data = chdata.second;
    QList<double> features;

    // TODO: Divide into subsignals
    features << data.squaredNorm(); // Compute variance

    return QPair<int,QList<double>>(chdata.first, features);
}


//*************************************************************************************************************

double BCI::applyClassificationCalcConcurrentlyOnSensorLevel(QList<double> &featData)
{
   double resultClassification = 0.5;

   return resultClassification;
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

bool BCI::hasThresholdArtefact(const MatrixXd &data)
{
    // Perform simple threshold artefact reduction
    double max = 0;
    double min = 0;

    // find min max in current m_matSlidingWindowSensor/qlMatrixRows after mean was subtracted
    max = data.maxCoeff();
    min = data.minCoeff();

//    cout<<"max: "<<max<<endl;
//    cout<<"min: "<<min<<endl;

    if(max<m_dThresholdValue*1e-06 && min>m_dThresholdValue*-1e-06) // If max is outside the threshold -> completley discard m_matSlidingWindowSensor
        return false;
    else
        return true;
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
                bool rejectTrial = false;
                for(int i = 0; i < m_matSlidingWindowSensor.rows(); i++)
                {
                    MatrixXd tempData = t_mat.block(m_mapElectrodePinningScheme[m_slChosenFeatureSensor.at(i)], 0, 1, t_mat.cols());

                    // Check for artifacts in selected channels
                    if(m_bUseArtefactThresholdReduction)
                    {
                        if(hasThresholdArtefact(tempData) == false)
                            m_matSlidingWindowSensor.block(i, m_iTBWIndexSensor, 1, t_mat.cols()) = tempData;
                        else
                            rejectTrial = true;
                    }
                    else
                        m_matSlidingWindowSensor.block(i, m_iTBWIndexSensor, 1, t_mat.cols()) = tempData;
                }

                if(rejectTrial == false)
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
                bool rejectTrial = false;
                for(int i = 0; i < m_matTimeBetweenWindowsSensor.rows(); i++)
                {
                    MatrixXd tempData = t_mat.block(m_mapElectrodePinningScheme[m_slChosenFeatureSensor.at(i)], 0, 1, t_mat.cols());

                    // Check for artifacts in selected channels
                    if(m_bUseArtefactThresholdReduction)
                    {
                        if(hasThresholdArtefact(tempData) == false)
                            m_matTimeBetweenWindowsSensor.block(i, m_iTBWIndexSensor, 1, t_mat.cols()) = tempData;
                        else
                            rejectTrial = true;
                    }
                    else
                        m_matTimeBetweenWindowsSensor.block(i, m_iTBWIndexSensor, 1, t_mat.cols()) = tempData;
                }

                if(rejectTrial == false)
                    m_iTBWIndexSensor = m_iTBWIndexSensor + t_mat.cols();
            }
            else // Recalculate m_matSlidingWindowSensor -> Calculate features, classify and store results
            {
                // ----1---- Recalculate m_matSlidingWindowSensor
                //cout<<"----1----"<<endl;
                //Delete same block size of data from the right -> use eval() to resolve Eigens aliasing problem
                m_matSlidingWindowSensor.block(0, 0, m_matSlidingWindowSensor.rows(), m_matSlidingWindowSensor.cols()-m_matTimeBetweenWindowsSensor.cols()) = m_matSlidingWindowSensor.block(0, m_matTimeBetweenWindowsSensor.cols(), m_matSlidingWindowSensor.rows(), m_matSlidingWindowSensor.cols()-m_matTimeBetweenWindowsSensor.cols()).eval();

                // push m_matTimeBetweenWindowsSensor from the left
                m_matSlidingWindowSensor.block(0, m_matSlidingWindowSensor.cols()-m_matTimeBetweenWindowsSensor.cols(), m_matTimeBetweenWindowsSensor.rows(), m_matTimeBetweenWindowsSensor.cols()) = m_matTimeBetweenWindowsSensor;

                // Test if data is correctly streamed to this plugin
                if(m_slChosenFeatureSensor.contains("TEST"))
                {
                    cout<<"Recalculate matrix"<<endl;
                    for(int i = 1; i<m_matSlidingWindowSensor.cols() ; i++)
                    {
                        if(m_matSlidingWindowSensor(m_matSlidingWindowSensor.rows()-1,i) - m_matSlidingWindowSensor(m_matSlidingWindowSensor.rows()-1,i-1) != 1)
                            cout<<"Sequence error while streaming from tmsi plugin at position: "<<i<<endl;
                    }
                }

                // ----2---- Transform matrix into QList structure, so that QTConurrent can handle it properly
                //cout<<"----2----"<<endl;
                QList<QPair<int,RowVectorXd>> qlMatrixRows;
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

                // ----4---- Filter data in m_matSlidingWindowSensor concurrently using map()
                //cout<<"----4----"<<endl;
                QList<QPair<int,RowVectorXd>> filteredRows = qlMatrixRows;

                if(m_bUseFilter)
                {
                    QFuture<void> futureFilter = QtConcurrent::map(filteredRows,[this](QPair<int,RowVectorXd>& chdata) {
                        applyFilterOperatorConcurrently(chdata);
                    });

                    futureFilter.waitForFinished();
                }

//                for(int i=0; i<m_matSlidingWindowSensor.rows(); i++)
//                    m_outStreamDebug << "m_matSlidingWindowSensor at row " << i << ": " << m_matSlidingWindowSensor.row(i) << "\n";

//                m_outStreamDebug<<endl;

//                for(int i=0; i<filteredRows.size(); i++)
//                    m_outStreamDebug << "filteredRows at row " << i << ": " << filteredRows.at(i).second << "\n";

//                m_outStreamDebug << endl << "---------------------------------------------------------" << endl << endl;

                // ----5---- Calculate features concurrently using mapped()
                //cout<<"----5----"<<endl;
                std::function<QPair<int,QList<double>> (QPair<int,RowVectorXd>&)> applyOpsFeatures = [this](QPair<int,RowVectorXd>& chdata) -> QPair<int,QList<double>> {
                    return applyFeatureCalcConcurrentlyOnSensorLevel(chdata);
                };

                QFuture<QPair<int,QList<double>>> futureCalculatedFeatures = QtConcurrent::mapped(filteredRows.begin(), filteredRows.end(), applyOpsFeatures);

                futureCalculatedFeatures.waitForFinished();

                m_iNumberOfCalculatedFeatures++;

                // ----6---- Store features
                //cout<<"----6----"<<endl;
                m_lFeaturesSensor.append(futureCalculatedFeatures.results());

                // ----7---- If enough features (windows) have been calculated (processed) -> classify all features and average result ----------
                //cout<<"----7----"<<endl;
                if(m_iNumberOfCalculatedFeatures >= (int)(m_matSlidingWindowSensor.cols()/m_matTimeBetweenWindowsSensor.cols()))
                {
                    // Transform m_lFeaturesSensor into an easier file structure
                    QList<QList<double>> lFeaturesSensor_new;

                    for(int i = 0; i<m_lFeaturesSensor.size()-iNumberOfFeatures+1; i = i + iNumberOfFeatures) // iterate over QPair feature List
                        for(int z = 0; z<m_lFeaturesSensor.at(0).second.size(); z++) // iterate over number of sub signals
                        {
                            QList<double> temp;
                            for(int t = 0; t<iNumberOfFeatures; t++) // iterate over chosen features (electrodes)
                                temp.append(m_lFeaturesSensor.at(i+t).second.at(z));
                            lFeaturesSensor_new.append(temp);
                        }

//                    //Check sizes
//                    cout<<"lFeaturesSensor_new.size()"<<lFeaturesSensor_new.size()<<endl;
//                    cout<<"lFeaturesSensor_new.at(0).size()"<<lFeaturesSensor_new.at(0).size()<<endl;
//                    cout<<"m_lFeaturesSensor.size()"<<m_lFeaturesSensor.size()<<endl;

                    // Display features
                    if(m_bTriggerActivated)
                        cout<<"Trigger activated"<<endl;

                    emit paintFeatures((MyQList)lFeaturesSensor_new, m_bTriggerActivated);
                    m_bTriggerActivated = false; // reset trigger

                    // ----8---- Classify features concurrently using mapped() ----------
                    //cout<<"----8----"<<endl;
                    std::function<double (QList<double>&)> applyOpsClassification = [this](QList<double>& featData){
                        return applyClassificationCalcConcurrentlyOnSensorLevel(featData);
                    };

                    QFuture<double> futureClassificationResults = QtConcurrent::mapped(lFeaturesSensor_new.begin(), lFeaturesSensor_new.end(), applyOpsClassification);

                    futureClassificationResults.waitForFinished();

                    // ----9---- Generate final classification result -> average all classification results
                    //cout<<"----9----"<<endl;
                    double dfinalResult = 0;

                    for(int i = 0; i<futureClassificationResults.resultCount() ;i++)
                        dfinalResult += futureClassificationResults.resultAt(i);

                    dfinalResult = dfinalResult/futureClassificationResults.resultCount();
                    //cout << dfinalResult << endl;

                    // ----10---- Store final result
                    //cout<<"----10----"<<endl;
                    m_lClassResultsSensor.append(dfinalResult);

                    // ----11---- Send result to the output stream, i.e. which is connected to the triggerbox
                    //cout<<"----11----"<<endl;
                    m_pBCIOutput->data()->setValue(dfinalResult);

                    // Clear classifications
                    clearFeatures();

                    // Reset counter
                    m_iNumberOfCalculatedFeatures = 0;
                }

                m_iTBWIndexSensor = 0;
            }
        }
    }
}
