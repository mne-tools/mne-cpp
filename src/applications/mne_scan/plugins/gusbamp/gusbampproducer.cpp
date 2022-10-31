//=============================================================================================================
/**
 * @file     gusbampproducer.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     November, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Christoph Dinh, Viktor Klueber, Lorenz Esch. All rights reserved.
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
 * @brief    Contains the implementation of the GUSBAmpProducer class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "gusbampproducer.h"
#include "gusbamp.h"
#include "gusbampdriver.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace GUSBAMPPLUGIN;
using namespace UTILSLIB;
using namespace Eigen;
using namespace std;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

GUSBAmpProducer::GUSBAmpProducer(GUSBAmp* pGUSBAmp)
: m_pGUSBAmp(pGUSBAmp)
, m_pGUSBAmpDriver(new GUSBAmpDriver(this))
, m_iSampRate(1200)
, m_sFilePath("data")
{
    m_viSizeOfSampleMatrix.resize(2,0);

    m_vSerials.resize(1);
    m_vSerials[0]= "UB-2015.05.16";
}

//=============================================================================================================

GUSBAmpProducer::~GUSBAmpProducer()
{
}

//=============================================================================================================

void GUSBAmpProducer::start(vector<QString> &serials,
                            vector<int> channels,
                            int sampleRate)
{
    //setting the new parameters of the gUSBamp device
    m_pGUSBAmpDriver->setSerials(serials);
    m_pGUSBAmpDriver->setSampleRate(sampleRate);
    m_pGUSBAmpDriver->setChannels(channels);

    //asking for the size of the sample Matrix which will be acquired
    m_viSizeOfSampleMatrix = m_pGUSBAmpDriver->getSizeOfSampleMatrix();

    //Initialise and starting the device
    if(m_pGUSBAmpDriver->initDevice()) {
        QThread::start();
    }
}

//=============================================================================================================

void GUSBAmpProducer::stop()
{
    requestInterruption();
    wait();
}

//=============================================================================================================

void GUSBAmpProducer::run()
{
    MatrixXf matData(m_viSizeOfSampleMatrix[0],m_viSizeOfSampleMatrix[1]);

    while(!isInterruptionRequested()) {
        //qDebug()<<"GUSBAmpProducer::run()"<<endl;
        //Get the GUSBAmp EEG data out of the device buffer and write received data to circular buffer
        if(m_pGUSBAmpDriver->getSampleMatrixValue(matData)) {
            while(!m_pGUSBAmp->m_pCircularBuffer->push(matData) && !isInterruptionRequested()) {
                //Do nothing until the circular buffer is ready to accept new data again
            }
        }
    }

    //Unitialise device only after the thread stopped
    m_pGUSBAmpDriver->uninitDevice();
}

//=============================================================================================================

vector<int> GUSBAmpProducer::getSizeOfSampleMatrix(void)
{
    return m_viSizeOfSampleMatrix;
}
