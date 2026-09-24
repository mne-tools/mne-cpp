//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2015-2026 MNE-CPP Authors
 *
 * @file     gusbampproducer.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     November, 2015
 * @brief    Contains the implementation of the GUSBAmpProducer class.
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
