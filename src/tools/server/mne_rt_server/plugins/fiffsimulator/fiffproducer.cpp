//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiffproducer.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 * @brief     Definition of the FiffProducer class.
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
    // Only meaningful once the read window has run past the end of the file,
    // which is also the only path that sets t_bRestart.
    fiff_int_t t_iDiff = 0;
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
            qInfo("error during read_raw_segment");
        }

        MatrixXf tmp = data.cast<float>();//(inv_calsMat*data).cast<float>();

        if(t_bRestart)
        {
            //
            // Case end of Simulation: restart file from the beginning and read remaining bytes
            //
            qInfo("### RESTART Simulation File ###\r");

            first = from;
            last = first+t_iDiff-1;

            if (!m_pFiffSimulator->m_RawInfo.read_raw_segment(data,times,first,last))
            {
                qInfo("error during read_raw_segment");
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
