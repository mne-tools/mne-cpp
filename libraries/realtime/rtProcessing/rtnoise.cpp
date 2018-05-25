//=============================================================================================================
/**
* @file     rtnoise.cpp
* @author   Limin Sun <liminsun@nmr.mgh.harvard.edu>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
*
* @version  1.0
* @date     August, 2014
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
* @brief     Definition of the RtNoise Class.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtnoise.h"

#include <iostream>
#include <fiff/fiff_cov.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace REALTIMELIB;
using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtNoise::RtNoise(qint32 p_iMaxSamples, FiffInfo::SPtr p_pFiffInfo, qint32 p_dataLen, QObject *parent)
: QThread(parent)
, m_iFFTlength(p_iMaxSamples)
, m_pFiffInfo(p_pFiffInfo)
, m_dataLength(p_dataLen)
, m_bIsRunning(false)
, m_iNumOfBlocks(0)
, m_iBlockSize(0)
, m_iSensors(0)
, m_iBlockIndex(0)
{
    qRegisterMetaType<Eigen::MatrixXd>("Eigen::MatrixXd");
    //qRegisterMetaType<QVector<double>>("QVector<double>");

    m_Fs = m_pFiffInfo->sfreq;

    m_bSendDataToBuffer = true;

    m_fWin.clear();

    //create a hanning window
    m_fWin = hanning(m_iFFTlength,0);

    qDebug()<<"Hanning window is created.";

}


//*************************************************************************************************************

RtNoise::~RtNoise()
{
    if(this->isRunning()){
        stop();
    }
}

//*************************************************************************************************************

QVector <float> RtNoise::hanning(int N, short itype)
{
    int half, i, idx, n;
    QVector <float> w(N,0.0);

    if(itype==1)    //periodic function
        n = N-1;
    else
        n = N;

    if(n%2==0)
    {
        half = n/2;
        for(i=0; i<half; i++) //CALC_HANNING   Calculates Hanning window samples.
            w[i] = 0.5 * (1 - cos(2*3.14159265*(i+1) / (n+1)));

        idx = half-1;
        for(i=half; i<n; i++) {
            w[i] = w[idx];
            idx--;
        }
    }
    else
    {
        half = (n+1)/2;
        for(i=0; i<half; i++) //CALC_HANNING   Calculates Hanning window samples.
            w[i] = 0.5 * (1 - cos(23.14159265*(i+1) / (n+1)));

        idx = half-2;
        for(i=half; i<n; i++) {
            w[i] = w[idx];
            idx--;
        }
    }

    if(itype==1)    //periodic function
    {
        for(i=N-1; i>=1; i--)
            w[i] = w[i-1];
        w[0] = 0.0;
    }
    return(w);
}

//*************************************************************************************************************

void RtNoise::append(const MatrixXd &p_DataSegment)
{
    if(!m_pRawMatrixBuffer)
        m_pRawMatrixBuffer = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(8, p_DataSegment.rows(), p_DataSegment.cols()));

    if (m_bSendDataToBuffer)
        m_pRawMatrixBuffer->push(&p_DataSegment);
}


//*************************************************************************************************************

bool RtNoise::start()
{
    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
    if(this->isRunning())
        QThread::wait();

    m_bIsRunning = true;
    QThread::start();

    return true;
}


//*************************************************************************************************************

bool RtNoise::stop()
{
    m_bIsRunning = false;

    m_pRawMatrixBuffer->releaseFromPop();

    m_pRawMatrixBuffer->clear();

    qDebug()<<" RtNoise Thread is stopped.";

    return true;
}


//*************************************************************************************************************

void RtNoise::run()
{
    bool FirstStart = true;

    while(m_bIsRunning)
    {
        if(m_pRawMatrixBuffer)
        {
            MatrixXd block = m_pRawMatrixBuffer->pop();

            if(FirstStart){
                //init the circ buffer and parameters
                if(m_dataLength < 0) m_dataLength = 10;
                m_iNumOfBlocks = m_dataLength;//60;
                m_iBlockSize =  block.cols();
                m_iSensors =  block.rows();

                m_matCircBuf.resize(m_iSensors,m_iNumOfBlocks*m_iBlockSize);

                m_iBlockIndex = 0;
                FirstStart = false;
            }
            //concate blocks
            for (int i=0; i< m_iSensors; i++)
                for (int j=0; j< m_iBlockSize; j++)
                    m_matCircBuf(i,j+m_iBlockIndex*m_iBlockSize) = block(i,j);


            m_iBlockIndex ++;
            if (m_iBlockIndex >= m_iNumOfBlocks){

                //m_pRawMatrixBuffer.clear(); //empty the buffer

                m_bSendDataToBuffer = false;
                //stop collect block and start to calculate the spectrum
                m_iBlockIndex = 0;

                MatrixXd sum_psdx = MatrixXd::Zero(m_iSensors,m_iFFTlength/2+1);

                int nb = floor(m_iNumOfBlocks*m_iBlockSize/m_iFFTlength)+1;
                qDebug()<<"nb"<<nb<<"NumOfBlocks"<<m_iNumOfBlocks<<"BlockSize"<<m_iBlockSize;
                MatrixXd t_mat(m_iSensors,m_iFFTlength);
                MatrixXd t_psdx(m_iSensors,m_iFFTlength/2+1);
                for (int n = 0; n<nb; n++){
                    //collect a data block with data length of m_iFFTlength;
                    if(n==nb-1)
                    {
                        for(qint32 ii=0; ii<m_iSensors; ii++)
                        for(qint32 jj=0; jj<m_iFFTlength; jj++)
                            if(jj+n*m_iFFTlength<m_iNumOfBlocks*m_iBlockSize)
                                t_mat(ii,jj) = m_matCircBuf(ii,jj+n*m_iFFTlength);
                            else
                                t_mat(ii,jj) = 0.0;

                    }
                    else
                    {
                        for(qint32 ii=0; ii<m_iSensors; ii++)
                        for(qint32 jj=0; jj<m_iFFTlength; jj++)
                            t_mat(ii,jj) = m_matCircBuf(ii,jj+n*m_iFFTlength);
                    }

                    //FFT calculation by row
                    for(qint32 i = 0; i < t_mat.rows(); i++){
                        RowVectorXd data;

                        data = t_mat.row(i);

                        //zero-pad data to m_iFFTlength
                        RowVectorXd t_dataZeroPad = RowVectorXd::Zero(m_iFFTlength);
                        t_dataZeroPad.head(data.cols()) = data;

                        for (qint32 lk = 0; lk<m_iFFTlength; lk++)
                            t_dataZeroPad[lk] = t_dataZeroPad[lk]*m_fWin[lk];

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
                        }
                     }//row computing is done
                }//nb

                //DB-calculation
                for(qint32 ii=0; ii<m_iSensors; ii++)
                    for(qint32 jj=0; jj<m_iFFTlength/2+1; jj++)
                        t_psdx(ii,jj) = 10.0*log10(sum_psdx(ii,jj)/nb);

                qDebug()<<"Send spectrum to Noise Estimator";
                emit SpecCalculated(t_psdx); //send back the spectrum result
                if(m_pRawMatrixBuffer->size()>0)
                    m_pRawMatrixBuffer->clear();

                m_bSendDataToBuffer = true;
            }

        }

    }

}

