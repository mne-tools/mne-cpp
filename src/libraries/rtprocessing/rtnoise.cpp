//=============================================================================================================
/**
 * @file     rtnoise.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     August, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Lorenz Esch, Christoph Dinh. All rights reserved.
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtnoise.h"

#include <iostream>
#include <fiff/fiff_cov.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTPROCESSINGLIB;
using namespace FIFFLIB;
using namespace Eigen;
using namespace UTILSLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtNoise::RtNoise(qint32 p_iMaxSamples,
                 FiffInfo::SPtr p_pFiffInfo,
                 qint32 p_dataLen,
                 QObject *parent)
: QThread(parent)
, m_iFftLength(p_iMaxSamples)
, m_pFiffInfo(p_pFiffInfo)
, m_dataLength(p_dataLen)
, m_bIsRunning(false)
, m_iNumOfBlocks(0)
, m_iBlockSize(0)
, m_iSensors(0)
, m_iBlockIndex(0)
{
    qRegisterMetaType<Eigen::MatrixXd>("Eigen::MatrixXd");
    //qRegisterMetaType<QVector<double> >("QVector<double>");

    m_Fs = m_pFiffInfo->sfreq;

    m_bSendDataToBuffer = true;

    m_fWin.clear();

    //create a hanning window
    m_fWin = hanning(m_iFftLength,0);

    qDebug()<<"Hanning window is created.";
}

//=============================================================================================================

RtNoise::~RtNoise()
{
    if(this->isRunning()){
        stop();
    }
}

//=============================================================================================================

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

//=============================================================================================================

void RtNoise::append(const MatrixXd &p_DataSegment)
{
    if(!m_pCircularBuffer)
        m_pCircularBuffer = CircularBuffer_Matrix_double::SPtr(new CircularBuffer_Matrix_double(8));

    if (m_bSendDataToBuffer)
        m_pCircularBuffer->push(p_DataSegment);
}

//=============================================================================================================

bool RtNoise::start()
{
    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
    if(this->isRunning())
        QThread::wait();

    m_bIsRunning = true;
    QThread::start();

    return true;
}

//=============================================================================================================

bool RtNoise::stop()
{
    m_bIsRunning = false;

    m_pCircularBuffer->clear();

    qDebug()<<" RtNoise Thread is stopped.";

    return true;
}

//=============================================================================================================

void RtNoise::run()
{
    #ifdef EIGEN_FFTW_DEFAULT
        fftw_make_planner_thread_safe();
    #endif

    bool FirstStart = true;
    MatrixXd block;

    while(m_bIsRunning) {
        if(m_pCircularBuffer) {
            if(m_pCircularBuffer->pop(block)) {
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

                    //m_pCircularBuffer.clear(); //empty the buffer

                    m_bSendDataToBuffer = false;
                    //stop collect block and start to calculate the spectrum
                    m_iBlockIndex = 0;

                    MatrixXd sum_psdx = MatrixXd::Zero(m_iSensors,m_iFftLength/2+1);

                    int nb = floor(m_iNumOfBlocks*m_iBlockSize/m_iFftLength)+1;
                    qDebug()<<"nb"<<nb<<"NumOfBlocks"<<m_iNumOfBlocks<<"BlockSize"<<m_iBlockSize;
                    MatrixXd t_mat(m_iSensors,m_iFftLength);
                    MatrixXd t_psdx(m_iSensors,m_iFftLength/2+1);
                    for (int n = 0; n<nb; n++){
                        //collect a data block with data length of m_iFftLength;
                        if(n==nb-1)
                        {
                            for(qint32 ii=0; ii<m_iSensors; ii++)
                            for(qint32 jj=0; jj<m_iFftLength; jj++)
                                if(jj+n*m_iFftLength<m_iNumOfBlocks*m_iBlockSize)
                                    t_mat(ii,jj) = m_matCircBuf(ii,jj+n*m_iFftLength);
                                else
                                    t_mat(ii,jj) = 0.0;

                        }
                        else
                        {
                            for(qint32 ii=0; ii<m_iSensors; ii++)
                            for(qint32 jj=0; jj<m_iFftLength; jj++)
                                t_mat(ii,jj) = m_matCircBuf(ii,jj+n*m_iFftLength);
                        }

                        //FFT calculation by row
                        for(qint32 i = 0; i < t_mat.rows(); i++){
                            RowVectorXd data;

                            data = t_mat.row(i);

                            //zero-pad data to m_iFftLength
                            RowVectorXd vecDataZeroPad = RowVectorXd::Zero(m_iFftLength);
                            vecDataZeroPad.head(data.cols()) = data;

                            for (qint32 lk = 0; lk<m_iFftLength; lk++)
                                vecDataZeroPad[lk] = vecDataZeroPad[lk]*m_fWin[lk];

                            //generate fft object
                            Eigen::FFT<double> fft;
                            fft.SetFlag(fft.HalfSpectrum);

                            //fft-transform data sequence
                            RowVectorXcd vecFreqData(m_iFftLength/2+1);
                            fft.fwd(vecFreqData,vecDataZeroPad);

                            // calculate spectrum from FFT
                            for(qint32 j=0; j<m_iFftLength/2+1;j++)
                            {
                                double mag_abs = sqrt(vecFreqData(j).real()* vecFreqData(j).real() +  vecFreqData(j).imag()*vecFreqData(j).imag());
                                double spower = (1.0/(m_Fs*m_iFftLength))* mag_abs;
                                if (j>0&&j<m_iFftLength/2) spower = 2.0*spower;
                                sum_psdx(i,j) = sum_psdx(i,j) + spower;
                            }
                         }//row computing is done
                    }//nb

                    //DB-calculation
                    for(qint32 ii=0; ii<m_iSensors; ii++)
                        for(qint32 jj=0; jj<m_iFftLength/2+1; jj++)
                            t_psdx(ii,jj) = 10.0*log10(sum_psdx(ii,jj)/nb);

                    qDebug()<<"Send spectrum to Noise Estimator";
                    emit SpecCalculated(t_psdx); //send back the spectrum result

                    m_bSendDataToBuffer = true;
                }
            }
        }

    }
}

