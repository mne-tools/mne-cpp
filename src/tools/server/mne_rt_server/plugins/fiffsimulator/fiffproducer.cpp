//=============================================================================================================
/**
 * @file     fiffproducer.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief     Definition of the FiffProducer class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffproducer.h"
#include "fiffsimulator.h"

#include <utils/generics/circularbuffer.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QFile>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace FIFFSIMULATORRTSERVERPLUGIN;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffProducer::FiffProducer(FiffSimulator* p_pFiffSimulator)
: m_pFiffSimulator(p_pFiffSimulator)
, m_bIsRunning(false)
{
}

//=============================================================================================================

FiffProducer::~FiffProducer()
{
    qDebug() << "Destroy FiffProducer::~FiffProducer()";

    stop();
}

//=============================================================================================================

bool FiffProducer::stop()
{
    m_bIsRunning = false;
    QThread::wait();

    return true;
}

//=============================================================================================================

void FiffProducer::run()
{
    m_bIsRunning = true;

    // reopen file in this thread
    QFile t_File(m_pFiffSimulator->m_RawInfo.info.filename);
    FiffStream::SPtr p_pStream(new FiffStream(&t_File));
    m_pFiffSimulator->m_RawInfo.file = p_pStream;

    //
    //   Set up the reading parameters
    //
    fiff_int_t from = m_pFiffSimulator->m_RawInfo.first_samp;
    fiff_int_t to = m_pFiffSimulator->m_RawInfo.last_samp;
//    float quantum_sec = (float)uiSamplePeriod/1000000.0f; //read and write in 10 sec junks
    fiff_int_t quantum = m_pFiffSimulator->m_uiBufferSampleSize;//ceil(quantum_sec*m_pFiffSimulator->m_pRawInfo->info.sfreq);

    qDebug() << "quantum " << quantum;

    //
    //   To read the whole file at once set
    //
    //quantum     = to - from + 1;
    //
    //
    //   Read and write all the data
    //

    fiff_int_t first, last;
    MatrixXd data;
    MatrixXd times;

    first = from;

//    //Calibration - Is taken care of during read_raw_segment(...) later in the code
//    qint32 nchan = m_pFiffSimulator->m_RawInfo.info.nchan;
//    MatrixXd cals(1,nchan);
//    SparseMatrix<double> inv_calsMat(nchan, nchan);
//    for(qint32 i = 0; i < nchan; ++i)
//        inv_calsMat.insert(i, i) = 1.0f/m_pFiffSimulator->m_RawInfo.info.chs[i].cal;

    //Not good cause production time is not accurate
    //loading and thread sleep is longer than thread sleep time - better to have a extra loading thread
    // ToDo restructure this producer as laoding buffer --> and thread sleep to simulator buffer
    fiff_int_t t_iDiff;
    bool t_bRestart = false;

    while(m_bIsRunning)
    {
        last = first+quantum-1;
        if (last > to)
        {
            t_iDiff = last - to;
            t_bRestart = true;

            last = to;
        }

        if (!m_pFiffSimulator->m_RawInfo.read_raw_segment(data,times,first,last))
        {
            printf("error during read_raw_segment\n");
        }

        MatrixXf tmp = data.cast<float>();//(inv_calsMat*data).cast<float>();

        if(t_bRestart)
        {
            //
            // Case end of Simulation: restart file from the beginning and read remaining bytes
            //
            printf("### RESTART Simulation File ###\r\n");

            first = from;
            last = first+t_iDiff-1;

            if (!m_pFiffSimulator->m_RawInfo.read_raw_segment(data,times,first,last))
            {
                printf("error during read_raw_segment\n");
            }

            MatrixXf tmp2 = data.cast<float>();//(inv_calsMat*data).cast<float>();

            MatrixXf tmp3(tmp.rows(), tmp.cols()+tmp2.cols());

            tmp3.block(0,0,tmp.rows(),tmp.cols()) = tmp;
            tmp3.block(0,tmp.cols(),tmp.rows(),tmp2.cols()) = tmp2;

            tmp = tmp3;

            t_bRestart = false;
            first += t_iDiff;
        }
        else
        {
            first += quantum;
        }

        // call blocks until there is free space in the buffer
        while(!m_pFiffSimulator->m_pRawMatrixBuffer->push(tmp) && m_bIsRunning) {
            //Do nothing until the circular buffer is ready to accept new data again
        }
    }

    // close datastream in this thread
//    delete m_pFiffSimulator->m_RawInfo.file;
//    m_pFiffSimulator->m_RawInfo.file = NULL;
}
