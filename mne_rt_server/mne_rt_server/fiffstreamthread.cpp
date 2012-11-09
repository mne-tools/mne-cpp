//=============================================================================================================
/**
* @file     fiffstreamthread.cpp
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
* @brief    Contains the implementation of the FiffStreamThread Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffstreamthread.h"
#include "fiffstreamserver.h"


#include "mne_rt_commands.h"

#include "../../MNE/fiff/fiff_constants.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtNetwork>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MSERVER;
using namespace FIFFLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffStreamThread::FiffStreamThread(qint32 id, int socketDescriptor, QObject *parent)
: QThread(parent)
, m_iDataClientId(id)
, m_sDataClientAlias(QString(""))
, socketDescriptor(socketDescriptor)
{
}


//*************************************************************************************************************

FiffStreamThread::~FiffStreamThread()
{
    //Remove from client list
    FiffStreamServer* t_pFiffStreamServer = qobject_cast<FiffStreamServer*>(this->parent());
    t_pFiffStreamServer->m_qClientList.remove(m_iDataClientId);
}


//*************************************************************************************************************

void FiffStreamThread::parseCommand(FiffTag* p_pTag)
{
    if(p_pTag->size() >= 4)
    {
        qint32* t_pInt = (qint32*)p_pTag->data();
        FiffTag::swap_intp(t_pInt);
        qint32 t_iCmd = t_pInt[0];

        if(t_iCmd == MNE_RT_SET_CLIENT_ALIAS)
        {
            //
            // Set Client Alias
            //
            m_sDataClientAlias = QString(p_pTag->mid(4, p_pTag->size()-4));
            printf("FiffStreamClient (ID %d): new alias = '%s'\r\n\n", m_iDataClientId, m_sDataClientAlias.toUtf8().constData());
        }
        else if(t_iCmd == MNE_RT_GET_CLIENT_ID)
        {
            //
            // Send Client ID
            //
            printf("FiffStreamClient (ID %d): send client ID %d\r\n\n", m_iDataClientId, m_iDataClientId);
            writeClientId();
        }
        else
        {
            printf("FiffStreamClient (ID %d): unknown command\r\n\n", m_iDataClientId);
        }
    }
    else
    {
        printf("FiffStreamClient (ID %d): unknown command\r\n\n", m_iDataClientId);
    }
}


//*************************************************************************************************************

void FiffStreamThread::getAndSendMeasurementInfo(qint32 ID, FiffInfo* p_pFiffInfo)
{
    if(ID == m_iDataClientId)
    {
        mutex.lock();



        FiffStream t_FiffStreamOut(&m_qSendBlock, QIODevice::WriteOnly);
        t_FiffStreamOut.setVersion(QDataStream::Qt_5_0);



//        qint32 init_info[2];
//        init_info[0] = FIFF_MNE_RT_CLIENT_ID;
//        init_info[1] = m_iDataClientId;

//        t_FiffStreamOut.start_block(FIFFB_MNE_RT_MEAS_INFO);










//FiffStream::start_writing_raw


        //
        //   We will always write floats
        //
        fiff_int_t data_type = 4;
        qint32 k;
        QList<FiffChInfo> chs;

        for(k = 0; k < p_pFiffInfo->nchan; ++k)
            chs << p_pFiffInfo->chs.at(k);

        fiff_int_t nchan = chs.size();

        //
        // write the essentials
        //
        t_FiffStreamOut.start_block(FIFFB_MEAS);//4
        t_FiffStreamOut.write_id(FIFF_BLOCK_ID);//5
        if(p_pFiffInfo->meas_id.version != -1)
        {
            t_FiffStreamOut.write_id(FIFF_PARENT_BLOCK_ID,p_pFiffInfo->meas_id);//6
        }
        //
        //    Measurement info
        //
        t_FiffStreamOut.start_block(FIFFB_MEAS_INFO);//7

        //
        //    Blocks from the original -> skip this
        //
//        QList<fiff_int_t> blocks;
//        blocks << FIFFB_SUBJECT << FIFFB_HPI_MEAS << FIFFB_HPI_RESULT << FIFFB_ISOTRAK << FIFFB_PROCESSING_HISTORY;
        bool have_hpi_result = false;
        bool have_isotrak    = false;
        //
        //    megacq parameters
        //
        if (!p_pFiffInfo->acq_pars.isEmpty() || !p_pFiffInfo->acq_stim.isEmpty())
        {
            t_FiffStreamOut.start_block(FIFFB_DACQ_PARS);
            if (!p_pFiffInfo->acq_pars.isEmpty())
                t_FiffStreamOut.write_string(FIFF_DACQ_PARS, p_pFiffInfo->acq_pars);

            if (!p_pFiffInfo->acq_stim.isEmpty())
                t_FiffStreamOut.write_string(FIFF_DACQ_STIM, p_pFiffInfo->acq_stim);

            t_FiffStreamOut.end_block(FIFFB_DACQ_PARS);
        }
        //
        //    Coordinate transformations if the HPI result block was not there
        //
        if (!have_hpi_result)
        {
            if (p_pFiffInfo->dev_head_t != NULL)
                t_FiffStreamOut.write_coord_trans(p_pFiffInfo->dev_head_t);

            if (p_pFiffInfo->ctf_head_t != NULL)
                t_FiffStreamOut.write_coord_trans(p_pFiffInfo->ctf_head_t);
        }
        //
        //    Polhemus data
        //
        if (p_pFiffInfo->dig.size() > 0 && !have_isotrak)
        {
            t_FiffStreamOut.start_block(FIFFB_ISOTRAK);
            for (qint32 k = 0; k < p_pFiffInfo->dig.size(); ++k)
                t_FiffStreamOut.write_dig_point(p_pFiffInfo->dig[k]);

            t_FiffStreamOut.end_block(FIFFB_ISOTRAK);
        }
        //
        //    Projectors
        //
        t_FiffStreamOut.write_proj(p_pFiffInfo->projs);
        //
        //    CTF compensation info
        //
        t_FiffStreamOut.write_ctf_comp(p_pFiffInfo->comps);
        //
        //    Bad channels
        //
        if (p_pFiffInfo->bads.size() > 0)
        {
            t_FiffStreamOut.start_block(FIFFB_MNE_BAD_CHANNELS);
            t_FiffStreamOut.write_name_list(FIFF_MNE_CH_NAME_LIST,p_pFiffInfo->bads);
            t_FiffStreamOut.end_block(FIFFB_MNE_BAD_CHANNELS);
        }
        //
        //    General
        //
        t_FiffStreamOut.write_float(FIFF_SFREQ,&p_pFiffInfo->sfreq);
        t_FiffStreamOut.write_float(FIFF_HIGHPASS,&p_pFiffInfo->highpass);
        t_FiffStreamOut.write_float(FIFF_LOWPASS,&p_pFiffInfo->lowpass);
        t_FiffStreamOut.write_int(FIFF_NCHAN,&nchan);
        t_FiffStreamOut.write_int(FIFF_DATA_PACK,&data_type);
        if (p_pFiffInfo->meas_date[0] != -1)
            t_FiffStreamOut.write_int(FIFF_MEAS_DATE,p_pFiffInfo->meas_date, 2);
        //
        //    Channel info
        //
        MatrixXd* cals = new MatrixXd(1,nchan);

        for(k = 0; k < nchan; ++k)
        {
            //
            //    Scan numbers may have been messed up
            //
            chs[k].scanno = k+1;//+1 because
            chs[k].range  = 1.0f;//Why? -> cause its already calibrated through reading
            (*cals)(0,k) = chs[k].cal;
            t_FiffStreamOut.write_ch_info(&chs[k]);
        }
        //
        //
        t_FiffStreamOut.end_block(FIFFB_MEAS_INFO);

        delete cals;

        mutex.unlock();

//        qDebug() << "MeasInfo Blocksize: " << m_qSendBlock.size();
    }
}


//*************************************************************************************************************

void FiffStreamThread::writeClientId()
{
    FiffStream t_FiffStreamOut(&m_qSendBlock, QIODevice::WriteOnly);
    t_FiffStreamOut.setVersion(QDataStream::Qt_5_0);

    t_FiffStreamOut.write_int(FIFF_MNE_RT_CLIENT_ID, &m_iDataClientId);
}


//*************************************************************************************************************

void FiffStreamThread::run()
{
    FiffStreamServer* parentServer = qobject_cast<FiffStreamServer*>(this->parent());
//    connect(parentServer, &FiffStreamServer::sendFiffStreamThreadInstruction,
//            this, &FiffStreamThread::readFiffStreamServerInstruction);

    connect(parentServer, &FiffStreamServer::sendMeasInfo,
            this, &FiffStreamThread::getAndSendMeasurementInfo);


    QTcpSocket t_qTcpSocket;
    if (!t_qTcpSocket.setSocketDescriptor(socketDescriptor)) {
        emit error(t_qTcpSocket.error());
        return;
    }
    else
    {
        printf("FiffStreamClient (assigned ID %d) accepted from\n\tIP:\t%s\n\tPort:\t%d\n\n",
               m_iDataClientId,
               QHostAddress(t_qTcpSocket.peerAddress()).toString().toUtf8().constData(),
               t_qTcpSocket.peerPort());
    }


    FiffStream t_FiffStreamIn(&t_qTcpSocket);
    t_FiffStreamIn.setVersion(QDataStream::Qt_5_0);

    while(t_qTcpSocket.state() != QAbstractSocket::UnconnectedState)
    {
        //
        // Write available data
        //
        if(m_qSendBlock.size() > 0)
        {
//            qDebug() << "is writeable " << t_qTcpSocket.isWritable();
            qint32 t_iBlockSize = m_qSendBlock.size();
//            qDebug() << "data available" << t_iBlockSize;
            mutex.lock();
            qint32 t_iBytesWritten = t_qTcpSocket.write(m_qSendBlock);
//            qDebug() << "wrote bytes " << t_iBytesWritten;
            t_qTcpSocket.waitForBytesWritten();
            if(t_iBytesWritten == t_iBlockSize)
            {
                m_qSendBlock.clear();
            }
            else
            {
                m_qSendBlock = m_qSendBlock.mid(t_iBytesWritten, t_iBlockSize-t_iBytesWritten);
            }
            mutex.unlock();
        }

        //
        // Read: Wait 10ms for incomming tag header, read and continue
        //
        t_qTcpSocket.waitForReadyRead(10);

        if (t_qTcpSocket.bytesAvailable() >= (int)sizeof(qint32)*4)
        {
            FiffTag* t_pTag = NULL;
            FiffTag::read_tag_info(&t_FiffStreamIn, t_pTag, false);

            //
            // wait until tag size data are available and read the data
            //
            while (t_qTcpSocket.bytesAvailable() < t_pTag->size())
            {
                t_qTcpSocket.waitForReadyRead(10);
            }
            FiffTag::read_tag_data(&t_FiffStreamIn, t_pTag);

            //
            // Parse the tag
            //
            if(t_pTag->kind == FIFF_MNE_RT_COMMAND)
            {
                parseCommand(t_pTag);
            }
        }
//        usleep(1000);
    }

    t_qTcpSocket.disconnectFromHost();
    if(t_qTcpSocket.state() != QAbstractSocket::UnconnectedState)
        t_qTcpSocket.waitForDisconnected();
}
