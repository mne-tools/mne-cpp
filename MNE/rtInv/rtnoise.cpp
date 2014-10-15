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
* @brief     implementation of the RtNoise Class.
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

using namespace RTINVLIB;
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
{
    qRegisterMetaType<Eigen::MatrixXd>("Eigen::MatrixXd");
    //qRegisterMetaType<QVector<double>>("QVector<double>");

    m_Fs = m_pFiffInfo->sfreq;

    SendDataToBuffer = true;
}


//*************************************************************************************************************

RtNoise::~RtNoise()
{
    if(this->isRunning())
        stop();
}


//*************************************************************************************************************

void RtNoise::append(const MatrixXd &p_DataSegment)
{
//    if(m_pRawMatrixBuffer) // ToDo handle change buffersize

    if(!m_pRawMatrixBuffer)
        m_pRawMatrixBuffer = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(8, p_DataSegment.rows(), p_DataSegment.cols()));

    if (SendDataToBuffer)
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
                NumOfBlocks = m_dataLength;//60;
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

                //m_pRawMatrixBuffer.clear(); //empty the buffer

                SendDataToBuffer = false;
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

                //DB-calculation
                for(qint32 ii=0; ii<Sensors; ii++)
                    for(qint32 jj=0; jj<m_iFFTlength/2+1; jj++)
                        t_psdx(ii,jj) = 10.0*log10(sum_psdx(ii,jj)/nb);

                qDebug()<<"Send spectrum to Noise Estimator";
                emit SpecCalculated(t_psdx); //send back the spectrum result
                if(m_pRawMatrixBuffer->size()>0)
                    m_pRawMatrixBuffer->clear();

                SendDataToBuffer = true;
            }

        }

    }

}

