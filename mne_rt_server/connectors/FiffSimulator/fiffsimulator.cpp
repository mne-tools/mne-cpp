//=============================================================================================================
/**
* @file     fiffsimulator.cpp
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
* @brief    Contains the implementation of the FiffSimulator Class.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffsimulator.h"
#include "fiffproducer.h"


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include "../../../MNE/fiff/fiff.h"
#include "../../../MNE/fiff/fiff_types.h"


//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include "../../../MNE/mne/mne.h"
#include "../../../MNE/mne/mne_epoch_data_list.h"


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

using namespace FiffSimulatorPlugin;
using namespace FIFFLIB;
using namespace MNELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffSimulator::FiffSimulator()
: m_pFiffProducer(new FiffProducer(this))
, m_pRawInfo(NULL)
, m_sResourceDataPath("../../mne-cpp/bin/MNE-sample-data/MEG/sample/sample_audvis_raw.fif")
{
    this->setBufferSampleSize(1000);
}


//*************************************************************************************************************

FiffSimulator::~FiffSimulator()
{
}


//*************************************************************************************************************

QByteArray FiffSimulator::availableCommands() const
{
    QByteArray t_blockCmdInfoList;

    t_blockCmdInfoList.append(QString("\t### %1 connector###\r\n").arg(this->getName()));

    t_blockCmdInfoList.append("\tsimfile  [file]\t\tthe fiff file which should be used as simulation file.\r\n");

    return t_blockCmdInfoList;
}


//*************************************************************************************************************

bool FiffSimulator::parseCommand(QStringList& p_sListCommand, QByteArray& p_blockOutputInfo)
{
    bool success = false;
    QByteArray t_blockClientList;
    if(p_sListCommand[0].compare("simfile",Qt::CaseInsensitive) == 0)
    {
        //
        // simulation file
        //
        printf("simfile\r\n");
        success = true;
    }

    return success;
}


//*************************************************************************************************************

bool FiffSimulator::start()
{
    init();

    // Start threads
    m_pFiffProducer->start();

    QThread::start();

    return true;
}


//*************************************************************************************************************

bool FiffSimulator::stop()
{
    QThread::wait();
    return true;
}


//*************************************************************************************************************

ConnectorID FiffSimulator::getConnectorID() const
{
    return _FIFFSIMULATOR;
}


//*************************************************************************************************************

void FiffSimulator::requestMeasInfo(qint32 ID)
{

    if(!m_pRawInfo)
        readRawInfo();

    if(m_pRawInfo)
        emit remitMeasInfo(ID, m_pRawInfo->info);
//    else
//        return NULL;
}


//*************************************************************************************************************

void FiffSimulator::requestRawData()
{
    this->m_pFiffProducer->start();
}


//*************************************************************************************************************

const char* FiffSimulator::getName() const
{
    return "Fiff File Simulator";
}


//*************************************************************************************************************

void FiffSimulator::init()
{

}


//*************************************************************************************************************

bool FiffSimulator::readRawInfo()
{
    if(!m_pRawInfo)
    {
        QFile* t_pFile = new QFile(m_sResourceDataPath);

        mutex.lock();
        if(!FiffStream::setup_read_raw(t_pFile, m_pRawInfo))
        {
            printf("Error: Not able to read raw info!\n");
            if(m_pRawInfo)
                delete m_pRawInfo;
            m_pRawInfo = NULL;

            delete t_pFile;
            return false;
        }

        //delete it here and reopen it in an other thread
        delete m_pRawInfo->file;
        m_pRawInfo->file = NULL;

        mutex.unlock();

        delete t_pFile;
    }

    return true;
}


//*************************************************************************************************************

void FiffSimulator::run()
{
    while(true)
    {

    }
}
