//=============================================================================================================
/**
* @file     fiffsimulator.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
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
* @brief     implementation of the FiffSimulator Class.
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
// INCLUDES
//=============================================================================================================

#include <stdlib.h>
#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>
#include <fiff/fiff_types.h>


//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include <mne/mne.h>
#include <mne/mne_epoch_data_list.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
#include <QFile>
#include <QCoreApplication>
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
// DEFINE MEMBER CONSTANTS
//=============================================================================================================

const QString FiffSimulator::Commands::BUFSIZE      = "bufsize";
const QString FiffSimulator::Commands::GETBUFSIZE   = "getbufsize";
const QString FiffSimulator::Commands::ACCEL        = "accel";
const QString FiffSimulator::Commands::GETACCEL     = "getaccel";
const QString FiffSimulator::Commands::SIMFILE      = "simfile";


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffSimulator::FiffSimulator()
: m_pFiffProducer(new FiffProducer(this))
, m_sResourceDataPath(QString("%1/MNE-sample-data/MEG/sample/sample_audvis_raw.fif").arg(QCoreApplication::applicationDirPath()))
, m_uiBufferSampleSize(512)//(4)
, m_AccelerationFactor(1.0)
, m_TrueSamplingRate(0.0)
, m_pRawMatrixBuffer(NULL)
, m_bIsRunning(false)
{
    this->init();
}


//*************************************************************************************************************

FiffSimulator::~FiffSimulator()
{
    qDebug() << "Destroy FiffSimulator::~FiffSimulator()";

    delete m_pFiffProducer;

    m_bIsRunning = false;
    QThread::wait();
}


//*************************************************************************************************************

void FiffSimulator::comBufsize(Command p_command)
{
    //ToDO JSON

    quint32 t_uiBuffSize = p_command.pValues()[0].toUInt();

    if(t_uiBuffSize > 0)
    {
//        printf("bufsize %d\n", t_uiBuffSize);

        bool t_bWasRunning = m_bIsRunning;

        if(m_bIsRunning)
        {
            m_pFiffProducer->stop();
            this->stop();
        }

        m_uiBufferSampleSize = t_uiBuffSize;

        if(t_bWasRunning)
            this->start();

        QString str = QString("\tSet %1 buffer sample size to %2 samples\r\n\n").arg(getName()).arg(t_uiBuffSize);

        m_commandManager[Commands::BUFSIZE].reply(str);
    }
    else
        m_commandManager[Commands::BUFSIZE].reply("Buffer size not set\r\n");
}


//*************************************************************************************************************

void FiffSimulator::comGetBufsize(Command p_command)
{
    bool t_bCommandIsJson = p_command.isJson();
    if(t_bCommandIsJson)
    {
        //
        //create JSON help object
        //
        QJsonObject t_qJsonObjectRoot;
        t_qJsonObjectRoot.insert(Commands::BUFSIZE, QJsonValue((double)m_uiBufferSampleSize));
        QJsonDocument p_qJsonDocument(t_qJsonObjectRoot);

        m_commandManager[Commands::GETBUFSIZE].reply(p_qJsonDocument.toJson());
    }
    else
    {
        QString str = QString("\t%1\r\n\n").arg(m_uiBufferSampleSize);
        m_commandManager[Commands::GETBUFSIZE].reply(str);
    }
}

//*************************************************************************************************************

void FiffSimulator::comAccel(Command p_command)
{
    //ToDO JSON

    float t_uiAccel = p_command.pValues()[0].toFloat();

    if(t_uiAccel > 0)
    {

            bool t_bWasRunning = m_bIsRunning;

            if(m_bIsRunning)
            {
                m_pFiffProducer->stop();
                this->stop();
            }

            m_AccelerationFactor = t_uiAccel;
            m_RawInfo.info.sfreq = m_AccelerationFactor * m_TrueSamplingRate;

            if(t_bWasRunning)
                this->start();

        QString str = QString("\tSet acceleration factor to %0.3f\r\n\n").arg(t_uiAccel);

        m_commandManager[Commands::ACCEL].reply(str);
    }
    else
        m_commandManager[Commands::ACCEL].reply("Acceleration facor not set\r\n");
}

//*************************************************************************************************************

void FiffSimulator::comGetAccel(Command p_command)
{
    bool t_bCommandIsJson = p_command.isJson();
    if(t_bCommandIsJson)
    {
        //
        //create JSON help object
        //
        QJsonObject t_qJsonObjectRoot;
        t_qJsonObjectRoot.insert(Commands::ACCEL, QJsonValue((double)m_AccelerationFactor));
        QJsonDocument p_qJsonDocument(t_qJsonObjectRoot);

        m_commandManager[Commands::GETACCEL].reply(p_qJsonDocument.toJson());
    }
    else
    {
        QString str = QString("\t%0.3f\r\n\n").arg(m_AccelerationFactor);
        m_commandManager[Commands::GETACCEL].reply(str);
    }
}

//*************************************************************************************************************

void FiffSimulator::comSimfile(Command p_command)
{
    //
    // simulation file
    //
    QFile t_file(p_command.pValues()[0].toString());

    QString t_sResourceDataPathOld = m_sResourceDataPath;

    if(t_file.exists())
    {
        m_sResourceDataPath = p_command.pValues()[0].toString();
        m_RawInfo = FiffRawData();

        if (this->readRawInfo())
        {
            m_pFiffProducer->stop();
            this->stop();

            m_commandManager[Commands::SIMFILE].reply("New simulation file set succefully.\r\n");
        }
        else
        {
            qDebug() << "Didn't set new file";
            m_sResourceDataPath = t_sResourceDataPathOld;

            m_commandManager[Commands::SIMFILE].reply("Simulation file not set.\r\n");
        }
    }
    else
    {
        qDebug() << "File does not exist on server!";
        m_sResourceDataPath = t_sResourceDataPathOld;
        m_commandManager[Commands::SIMFILE].reply("Simulation file not set.\r\n");
    }
}


//*************************************************************************************************************

void FiffSimulator::connectCommandManager()
{
    //Connect slots
    QObject::connect(&m_commandManager[Commands::BUFSIZE], &Command::executed, this, &FiffSimulator::comBufsize);
    QObject::connect(&m_commandManager[Commands::GETBUFSIZE], &Command::executed, this, &FiffSimulator::comGetBufsize);
    QObject::connect(&m_commandManager[Commands::ACCEL], &Command::executed, this, &FiffSimulator::comAccel);
    QObject::connect(&m_commandManager[Commands::GETACCEL], &Command::executed, this, &FiffSimulator::comGetAccel);
    QObject::connect(&m_commandManager[Commands::SIMFILE], &Command::executed, this, &FiffSimulator::comSimfile);
}


//*************************************************************************************************************

ConnectorID FiffSimulator::getConnectorID() const
{
    return _FIFFSIMULATOR;
}


//*************************************************************************************************************

const char* FiffSimulator::getName() const
{
    return "Fiff File Simulator";
}


//*************************************************************************************************************

void FiffSimulator::init()
{
    //
    // Read cfg file
    //
    QFile t_qFile(QString("%1/mne_rt_server_plugins/FiffSimulation.cfg").arg(QCoreApplication::applicationDirPath()));
    if (t_qFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&t_qFile);
        QString key = "simFile = ";
        while (!in.atEnd()) {
            QString line = in.readLine();
            if(line.contains(key, Qt::CaseInsensitive))
            {
                qint32 idx = line.indexOf(key);
                idx += key.size();

                QString sFileName = line.mid(idx, line.size()-idx);

                QFile t_qFileMeas(sFileName);

                if (t_qFileMeas.open(QIODevice::ReadOnly))
                {
                    m_sResourceDataPath = sFileName;
                    std::cout << "\tLoad simulation file: " << sFileName.toLatin1().constData() << std::endl;
                    t_qFileMeas.close();
                }
            }
        }
        t_qFile.close();
    }

    if(m_pRawMatrixBuffer)
        delete m_pRawMatrixBuffer;
    m_pRawMatrixBuffer = NULL;

    if(!m_RawInfo.isEmpty())
        m_pRawMatrixBuffer = new RawMatrixBuffer(RAW_BUFFFER_SIZE, m_RawInfo.info.nchan, this->m_uiBufferSampleSize);
}


//*************************************************************************************************************

bool FiffSimulator::start()
{
    this->init();

    // Start threads
    m_pFiffProducer->start();

    QThread::start();

    return true;
}


//*************************************************************************************************************

bool FiffSimulator::stop()
{
    this->m_pFiffProducer->stop();
    m_bIsRunning = false;
    QThread::wait();

    return true;
}


//*************************************************************************************************************

void FiffSimulator::info(qint32 ID)
{

    if(m_RawInfo.isEmpty())
        readRawInfo();

    if(!m_RawInfo.isEmpty())
        emit remitMeasInfo(ID, m_RawInfo.info);
}


//*************************************************************************************************************

bool FiffSimulator::readRawInfo()
{
    if(m_RawInfo.isEmpty())
    {
        QFile t_File(m_sResourceDataPath);

        mutex.lock();

        if(!FiffStream::setup_read_raw(t_File, m_RawInfo))
        {
            printf("Error: Not able to read raw info!\n");
            m_RawInfo.clear();
            return false;
        }

        m_TrueSamplingRate = m_RawInfo.info.sfreq;
        m_RawInfo.info.sfreq *= m_AccelerationFactor;

//        bool in_samples = false;
//
//        bool keep_comp = true;
//
//        //
//        //   Set up pick list: MEG + STI 014 - bad channels
//        //
//        //
//        QStringList include;
//        include << "STI 014";
//        bool want_meg   = true;
//        bool want_eeg   = true;
//        bool want_stim  = true;

//    //    MatrixXi picks = Fiff::pick_types(m_RawInfo.info, want_meg, want_eeg, want_stim, include, m_RawInfo.info.bads);
//        MatrixXi picks = m_RawInfo.info.pick_types(want_meg, want_eeg, want_stim, include, m_RawInfo.info.bads); //Prefer member function


//        //
//        //   Set up projection
//        //
//        qint32 k = 0;
//        if (m_RawInfo.info.projs.size() == 0)
//            printf("No projector specified for these data\n");
//        else
//        {
//            //
//            //   Activate the projection items
//            //
//            for (k = 0; k < m_RawInfo.info.projs.size(); ++k)
//                m_RawInfo.info.projs[k].active = true;

//            printf("%d projection items activated\n",m_RawInfo.info.projs.size());
//            //
//            //   Create the projector
//            //
//    //        fiff_int_t nproj = MNE::make_projector_info(m_RawInfo.info, m_RawInfo.proj); Using the member function instead
//            fiff_int_t nproj = m_RawInfo.info.make_projector_info(m_RawInfo.proj);


//    //        qDebug() << m_RawInfo.proj.data->data.rows();
//    //        qDebug() << m_RawInfo.proj.data->data.cols();
//    //        std::cout << "proj: \n" << m_RawInfo.proj.data->data.block(0,0,10,10);

//            if (nproj == 0)
//            {
//                printf("The projection vectors do not apply to these channels\n");
//            }
//            else
//            {
//                printf("Created an SSP operator (subspace dimension = %d)\n",nproj);
//            }
//        }

//        //
//        //   Set up the CTF compensator
//        //
//    //    qint32 current_comp = MNE::get_current_comp(m_RawInfo.info);
//        qint32 current_comp = m_RawInfo.info.get_current_comp();
//        qint32 dest_comp = -1;

//        if (current_comp > 0)
//            printf("Current compensation grade : %d\n",current_comp);

//        if (keep_comp)
//            dest_comp = current_comp;

//        if (current_comp != dest_comp)
//        {
//            qDebug() << "This part needs to be debugged";
//            if(MNE::make_compensator(*m_RawInfo.info.data(), current_comp, dest_comp, m_RawInfo.comp))
//            {
//    //            m_RawInfo.info.chs = MNE::set_current_comp(m_RawInfo.info.chs,dest_comp);
//                m_RawInfo.info.set_current_comp(dest_comp);
//                printf("Appropriate compensator added to change to grade %d.\n",dest_comp);
//            }
//            else
//            {
//                printf("Could not make the compensator\n");
//                return -1;
//            }
//        }

        //
        // Create circular buffer to transfer data form producer to simulator
        //
        if(m_pRawMatrixBuffer)
            delete m_pRawMatrixBuffer;
        m_pRawMatrixBuffer = new RawMatrixBuffer(10, m_RawInfo.info.nchan, m_uiBufferSampleSize);

        mutex.unlock();
    }

    return true;
}


//*************************************************************************************************************

void FiffSimulator::run()
{
    m_bIsRunning = true;

    float t_fSamplingFrequency = m_RawInfo.info.sfreq;
    float t_fBuffSampleSize = (float)m_uiBufferSampleSize;

    quint32 uiSamplePeriod = (unsigned int) ((t_fBuffSampleSize/t_fSamplingFrequency)*1000000.0f);

//    quint32 count = 0;

    while(m_bIsRunning)
    {
        QSharedPointer<Eigen::MatrixXf> t_pRawBuffer(new Eigen::MatrixXf(m_pRawMatrixBuffer->pop()));
//        ++count;
//        printf("%d raw buffer (%d x %d) generated\r\n", count, t_pRawBuffer->rows(), t_pRawBuffer->cols());

        emit remitRawBuffer(t_pRawBuffer);
        usleep(uiSamplePeriod);
    }
}
