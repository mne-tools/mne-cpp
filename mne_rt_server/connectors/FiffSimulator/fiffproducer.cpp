//=============================================================================================================
/**
* @file     fiffproducer.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hämäläinen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the DataProducer Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffproducer.h"



//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FiffSimulatorPlugin;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffProducer::FiffProducer()
: m_bIsRunning(false)
{

}


//*************************************************************************************************************

FiffProducer::~FiffProducer()
{

}


//*************************************************************************************************************

void FiffProducer::stop()
{
    m_bIsRunning = false;
    QThread::wait();
}


//*************************************************************************************************************

void FiffProducer::run()
{
    unsigned int uiSamplePeriod = (unsigned int) (1000000.0/6);//(m_pDataSimulator->m_fSamplingRate));

    unsigned int count = 0;

    m_bIsRunning = true;

    while(m_bIsRunning)
    {
        usleep(uiSamplePeriod);

        qDebug() << "Data package " << count << " produced";












//        //
//        //   Set up the reading parameters
//        //
//        fiff_int_t from = raw->first_samp;
//        fiff_int_t to = raw->last_samp;
//        float quantum_sec = 10.0f;//read and write in 10 sec junks
//        fiff_int_t quantum = ceil(quantum_sec*raw->info->sfreq);
//        //
//        //   To read the whole file at once set
//        //
//        //quantum     = to - from + 1;
//        //
//        //
//        //   Read and write all the data
//        //
//        bool first_buffer = true;

//        fiff_int_t first, last;
//        MatrixXd* data = NULL;
//        MatrixXd* times = NULL;

//        //This is the while loop
//        for(first = from; first < to; first+=quantum)
//        {
//            last = first+quantum-1;
//            if (last > to)
//            {
//                last = to;
//            }

//            if (!raw->read_raw_segment(data,times,first,last,picks))
//            {
//                    delete raw;
//                    delete outfid;
//                    printf("error during read_raw_segment\n");
//                    return -1;
//            }
//            //
//            //   You can add your own miracle here
//            //
//            printf("Writing...");
//            if (first_buffer)
//            {
//               if (first > 0)
//                   outfid->write_int(FIFF_FIRST_SAMPLE,&first);
//               first_buffer = false;
//            }
//            outfid->write_raw_buffer(data,cals);
//            printf("[done]\n");
//        }




















        ++count;
    }
}
