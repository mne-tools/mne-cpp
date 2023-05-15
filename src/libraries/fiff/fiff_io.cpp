//=============================================================================================================
/**
 * @file     fiff_io.cpp
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
 * @brief    Definition of a generic Fiff IO interface
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_io.h"
#include "fiff.h"
#include "fiff_evoked_set.h"
#include "fiff_stream.h"

#include <rtprocessing/filter.h>
#include <rtprocessing/helpers/filterkernel.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace Eigen;
using namespace RTPROCESSINGLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffIO::FiffIO()
{
}

//=============================================================================================================

FiffIO::~FiffIO()
{
}

//=============================================================================================================

FiffIO::FiffIO(QIODevice& pIODevice)
{
    // execute read method
    FiffIO::read(pIODevice);
}

//=============================================================================================================

FiffIO::FiffIO(QList<QIODevice*>& p_qlistIODevices)
{
    QList<QIODevice*>::iterator i;
    for(i = p_qlistIODevices.begin(); i != p_qlistIODevices.end(); ++i) {
        FiffIO::read((**i));
    }
}

//=============================================================================================================

bool FiffIO::setup_read(QIODevice& pIODevice,
                        FiffInfo& info,
                        FiffDirNode::SPtr& dirTree)
{
    //Open the file
    FiffStream::SPtr p_pStream(new FiffStream(&pIODevice));
    QString t_sFileName = p_pStream->streamName();

    printf("Opening fiff data %s...\n",t_sFileName.toUtf8().constData());

    if(!p_pStream->open())
        return false;

    //Read the measurement info
    if(!p_pStream->read_meas_info(p_pStream->dirtree(), info, dirTree))
        return false;

    return true;
}

//=============================================================================================================

bool FiffIO::read(QIODevice& pIODevice)
{
    //Read dirTree from fiff data (raw,evoked,fwds,cov)
    FiffInfo t_fiffInfo;
    FiffDirNode::SPtr t_dirTree;
    bool hasRaw = false;
    bool hasEvoked = false; // hasFwds=false;

    FiffIO::setup_read(pIODevice, t_fiffInfo, t_dirTree);
    pIODevice.close(); //file can be closed, since IODevice is already read

    if(!t_dirTree) {
        qWarning() << "[FiffIO::read] Dir tree could not be read";
        return false;
    }

    //Search dirTree for specific data types
    if(t_dirTree->has_kind(FIFFB_EVOKED)) {
        hasEvoked = true;
    }

    if(t_dirTree->has_kind(FIFFB_RAW_DATA) ||
       t_dirTree->has_kind(FIFFB_PROCESSED_DATA) ||
       t_dirTree->has_kind(FIFFB_CONTINUOUS_DATA) ||
       t_dirTree->has_kind(FIFFB_SMSH_RAW_DATA)) {
        hasRaw = true;
    }

   // if(t_Tree.has_kind(FIFFB_MNE_FORWARD_SOLUTION))
   //     hasFwds = true;

    //Read all sort of types
    //raw data
    if(hasRaw) {
        QSharedPointer<FiffRawData> p_fiffRawData(new FiffRawData(pIODevice));
        pIODevice.close();

        //append to corresponding member qlist
        m_qlistRaw.append(p_fiffRawData);

        printf( "Finished reading raw data!\n");
    }

    //evoked data + projections
    if(hasEvoked) {
        FiffEvokedSet p_fiffEvokedSet(pIODevice);
        pIODevice.close();

        //append to corresponding member qlist
        for(qint32 i=0; i < p_fiffEvokedSet.evoked.size(); ++i) {
            m_qlistEvoked.append(QSharedPointer<FiffEvoked>(&p_fiffEvokedSet.evoked[i]));
        }
    }

//    //forward solutions
//    if(hasFwds) {
//        MNEForwardSolution p_forwardSolution(pIODevice);

//        //append to corresponding member qlist
//        m_qlistFwd.append(QSharedPointer<MNEForwardSolution>(&p_forwardSolution));
//    }

    //print summary
    //std::cout << *this << std::endl;

    return true;
}

//=============================================================================================================

bool FiffIO::write(QIODevice& pIODevice,
                   const fiff_int_t type,
                   const fiff_int_t idx) const {
    switch(type) {
        case FIFFB_RAW_DATA: {
            FiffIO::write_raw(pIODevice,idx);
            qDebug() << "Finished writing single raw data with index" << idx << ".";
        }
        case FIFFB_EVOKED:
        //ToDo: write evoked set to file
        ;
    }

    return true;
}

//=============================================================================================================

bool FiffIO::write(QFile& p_QFile,
                   const fiff_int_t type,
                   const fiff_int_t idx) const {
    qDebug("------------------------ Writing fiff data ------------------------");

    switch(type) {
        case FIFFB_RAW_DATA: {
        QString t_nameoftype = "raw";

        if(idx == -1) {
            for(qint32 i=0; i < m_qlistRaw.size(); ++i) {
                QString t_fname;
                //insert
                qint32 p = p_QFile.fileName().indexOf(".fif");
                t_fname = p_QFile.fileName().insert(p,QString("_"+t_nameoftype+"-"+QString::number(i)));

                //assign new file name
                qDebug("\nWriting set with index %i to file %s ...",i,t_fname.toUtf8().constData());
                QFile t_file(t_fname);

                FiffIO::write_raw(t_file,i);
            }
        }
        else {
            FiffIO::write_raw(p_QFile,idx);
        }
        qDebug("\nFinished Writing %lli raw data sets!\n",m_qlistRaw.size());
        }
        case FIFFB_EVOKED:

        //ToDo: write evoked set to file
        ;
    }

    return true;
}

//=============================================================================================================

bool FiffIO::write_raw(QIODevice &pIODevice,
                       const fiff_int_t idx) const
{
    RowVectorXd cals;
    SparseMatrix<double> mult;
    RowVectorXi sel;
    FiffStream::SPtr outfid = FiffStream::start_writing_raw(pIODevice, this->m_qlistRaw[idx]->info, cals);

    //Setup reading parameters
    fiff_int_t from = m_qlistRaw[idx]->first_samp;
    fiff_int_t to = m_qlistRaw[idx]->last_samp;
    float quantum_sec = 30.0f;//read and write in 30 sec junks
    fiff_int_t quantum = ceil(quantum_sec*m_qlistRaw[idx]->info.sfreq);

    // Uncomment to read the whole file at once. Warning Matrix may be none-initialisable because its huge
    //quantum = to - from + 1;

    // Read and write all the data
    bool first_buffer = true;

    fiff_int_t first, last;
    MatrixXd data;
    MatrixXd times;

    for(first = from; first < to; first+=quantum) {
        last = first+quantum-1;
        if (last > to)
            last = to;

        if (!m_qlistRaw[idx]->read_raw_segment(data, times, mult, first, last, sel)) {
            qDebug("error during read_raw_segment\n");
            return false;
        }

        qDebug("Writing...");
        if (first_buffer) {
           if (first > 0)
               outfid->write_int(FIFF_FIRST_SAMPLE,&first);
           first_buffer = false;
        }
        outfid->write_raw_buffer(data, cals);
        qDebug("[done]\n");
    }

    outfid->finish_writing_raw();

    return true;
}
