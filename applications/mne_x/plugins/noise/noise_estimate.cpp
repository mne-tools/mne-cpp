//=============================================================================================================
/**
* @file     noise_estimate.cpp
* @author   Limin Sun <liminsun@nmr.mgh.harvard.edu>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Limin Sun, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the NoiseEstimate class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "noise_estimate.h"
#include "FormFiles/noiseestimatesetupwidget.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace NoiseEstimatePlugin;
using namespace MNEX;
using namespace XMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NoiseEstimate::NoiseEstimate()
: m_bIsRunning(false)
, m_bProcessData(false)
, m_pRTMSAInput(NULL)
, m_pFSOutput(NULL)
, m_pBuffer(CircularMatrixBuffer<double>::SPtr())
, m_Fs(600)
, m_iFFTlength(4096)
{
}


//*************************************************************************************************************

NoiseEstimate::~NoiseEstimate()
{
    if(this->isRunning())
        stop();
}


//*************************************************************************************************************

QSharedPointer<IPlugin> NoiseEstimate::clone() const
{
    QSharedPointer<NoiseEstimate> pNoiseEstimateClone(new NoiseEstimate);
    return pNoiseEstimateClone;
}


//*************************************************************************************************************
//=============================================================================================================
// Creating required display instances and set configurations
//=============================================================================================================

void NoiseEstimate::init()
{
    // Input
    m_pRTMSAInput = PluginInputData<NewRealTimeMultiSampleArray>::create(this, "Noise Estimatge In", "Noise Estimate input data");
    connect(m_pRTMSAInput.data(), &PluginInputConnector::notify, this, &NoiseEstimate::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTMSAInput);

    // Output
    m_pFSOutput = PluginOutputData<FrequencySpectrum>::create(this, "Noise Estimate Out", "Noise Estimate output data");
    m_outputConnectors.append(m_pFSOutput);

//    m_pRTMSAOutput->data()->setMultiArraySize(100);
//    m_pRTMSAOutput->data()->setVisibility(true);

    //init channels when fiff info is available
    connect(this, &NoiseEstimate::fiffInfoAvailable, this, &NoiseEstimate::initConnector);

    //Delete Buffer - will be initailzed with first incoming data
    if(!m_pBuffer.isNull())
        m_pBuffer = CircularMatrixBuffer<double>::SPtr();
}


//*************************************************************************************************************

void NoiseEstimate::unload()
{

}


//*************************************************************************************************************

void NoiseEstimate::initConnector()
{
    qDebug() << "void NoiseEstimate::initConnector()";
    if(m_pFiffInfo)
        m_pFSOutput->data()->initFromFiffInfo(m_pFiffInfo);
}


//*************************************************************************************************************

bool NoiseEstimate::start()
{
    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
    if(this->isRunning())
        QThread::wait();

    m_bIsRunning = true;

    // Start threads
    QThread::start();

    return true;
}


//*************************************************************************************************************

bool NoiseEstimate::stop()
{
    //Wait until this thread is stopped
    m_bIsRunning = false;

    if(m_bProcessData)
    {
        //In case the semaphore blocks the thread -> Release the QSemaphore and let it exit from the pop function (acquire statement)
        m_pBuffer->releaseFromPop();
        m_pBuffer->releaseFromPush();

        m_pBuffer->clear();

//        m_pNEOutput->data()->clear();
    }

    return true;
}


//*************************************************************************************************************

IPlugin::PluginType NoiseEstimate::getType() const
{
    return _IAlgorithm;
}


//*************************************************************************************************************

QString NoiseEstimate::getName() const
{
    return "NoiseEstimate Toolbox";
}


//*************************************************************************************************************

QWidget* NoiseEstimate::setupWidget()
{
    NoiseEstimateSetupWidget* setupWidget = new NoiseEstimateSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new

    connect(this,&NoiseEstimate::SetNoisePara,setupWidget,&NoiseEstimateSetupWidget::init);
    //connect(this,&NoiseEstimate::RePlot,setupWidget,&NoiseEstimateSetupWidget::Update);

    return setupWidget;

}


//*************************************************************************************************************

void NoiseEstimate::update(XMEASLIB::NewMeasurement::SPtr pMeasurement)
{
    QSharedPointer<NewRealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<NewRealTimeMultiSampleArray>();

    if(pRTMSA)
    {
        //Check if buffer initialized
        if(!m_pBuffer)
            m_pBuffer = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(64, pRTMSA->getNumChannels(), pRTMSA->getMultiArraySize()));

        //Fiff information
        if(!m_pFiffInfo)
        {
            m_pFiffInfo = pRTMSA->getFiffInfo();
            emit fiffInfoAvailable();
        }

        if(m_bProcessData)
        {
            MatrixXd t_mat(pRTMSA->getNumChannels(), pRTMSA->getMultiArraySize());

            for(qint32 i = 0; i < pRTMSA->getMultiArraySize(); ++i)
                t_mat.col(i) = pRTMSA->getMultiSampleArray()[i];

            m_pBuffer->push(&t_mat);
        }
    }
}



//*************************************************************************************************************

void NoiseEstimate::run()
{
    //
    // Read Fiff Info
    //
    while(!m_pFiffInfo)
        msleep(10);// Wait for fiff Info

    m_bProcessData = true;

    qDebug() << "RUN m_iFFTlength" << m_iFFTlength;
    m_Fs = m_pFiffInfo->sfreq;

    bool FirstStart = true;

    while (m_bIsRunning)
    {
        if(m_bProcessData)
        {
            /* Dispatch the inputs */
            MatrixXd block = m_pBuffer->pop();

            //ToDo: Implement your algorithm here

            if(FirstStart){
                //init the circ buffer and parameters
                NumOfBlocks = 60;
                BlockSize =  block.cols();
                Sensors =  block.rows();

                CircBuf.resize(Sensors,NumOfBlocks*BlockSize);

                BlockIndex = 0;
                FirstStart = false;
            }
            //concate blocks
            for (int i=0; i< Sensors; i++)
                for (int j=0; j< BlockSize; j++)
                    CircBuf(i,j+BlockIndex*BlockSize) = block(i,j);

            BlockIndex ++;
            if (BlockIndex >= NumOfBlocks){
                //stop collect block and start to calculate the spectrum
                BlockIndex = 0;
                MatrixXd sum_psdx = MatrixXd::Zero(Sensors,m_iFFTlength/2+1);

                int nb = floor(NumOfBlocks*BlockSize/m_iFFTlength)+1;
                qDebug()<<"nb"<<nb<<"NumOfBlocks"<<NumOfBlocks<<"BlockSize"<<BlockSize;
                MatrixXd t_mat(Sensors,m_iFFTlength);
                MatrixXd t_psdx(Sensors,m_iFFTlength/2+1);

                for (int n = 0; n<nb; n++){
                    //collect a data block with data length of m_iFFTlength;
                    if(n==nb-1)
                    {
                        for(qint32 ii=0; ii<Sensors; ii++)
                        for(qint32 jj=0; jj<m_iFFTlength; jj++)
                            if(jj+n*m_iFFTlength<NumOfBlocks*BlockSize)
                                t_mat(ii,jj) = CircBuf(ii,jj+n*m_iFFTlength);
                            else
                                t_mat(ii,jj) = 0.0;

                    }
                    else
                    {
                        for(qint32 ii=0; ii<Sensors; ii++)
                        for(qint32 jj=0; jj<m_iFFTlength; jj++)
                            t_mat(ii,jj) = CircBuf(ii,jj+n*m_iFFTlength);
                    }
                    //FFT calculation by row
                    for(qint32 i = 0; i < t_mat.rows(); i++){
                        RowVectorXd data;

                        data = t_mat.row(i);

                        //zero-pad data to m_iFFTlength
                        RowVectorXd t_dataZeroPad = RowVectorXd::Zero(m_iFFTlength);
                        t_dataZeroPad.head(data.cols()) = data;

                        //generate fft object
                        Eigen::FFT<double> fft;
                        fft.SetFlag(fft.HalfSpectrum);

                        //fft-transform data sequence
                        RowVectorXcd t_freqData(m_iFFTlength/2+1);
                        fft.fwd(t_freqData,t_dataZeroPad);

                        // calculate spectrum from FFT
                        for(qint32 j=0; j<m_iFFTlength/2+1;j++)
                        {
                            double mag_abs = sqrt(t_freqData(j).real()* t_freqData(j).real() +  t_freqData(j).imag()*t_freqData(j).imag());
                            double spower = (1.0/(m_Fs*m_iFFTlength))* mag_abs;
                            if (j>0&&j<m_iFFTlength/2) spower = 2.0*spower;
                            sum_psdx(i,j) = sum_psdx(i,j) + spower;
                //            t_psdx(i,j) = 10.0*log10(sum_psdx(i,j)/ncount);
                        }
                     }//row computing is done
                }//nb

                for(qint32 ii=0; ii<Sensors; ii++)
                    for(qint32 jj=0; jj<m_iFFTlength/2+1; jj++)
                        t_psdx(ii,jj) =  10.0*log10(sum_psdx(ii,jj)/nb);

                qDebug()<< "Spec" << sum_psdx(0,1) << t_psdx(0,1) << nb;
                //send spectrum to the output data
                m_pFSOutput->data()->setValue(t_psdx);


            }//next turn for n blocks data colloection

        }//m_bProcessData
    }//m_bIsRunning
}

