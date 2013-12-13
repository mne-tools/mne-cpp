//=============================================================================================================
/**
* @file     fiff_obj.cpp
* @author   Florian Schlembach <florian.schlembach@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
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
* @brief    Implementation of a generic Fiff IO interface
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_io.h"
#include "fiff_stream.h"
#include <mne/mne.h>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffIO::FiffIO()
{
}

//*************************************************************************************************************

FiffIO::~FiffIO()
{
}

//*************************************************************************************************************

FiffIO::FiffIO(QIODevice& p_IODevice)
{
    // execute read method
    FiffIO::read(p_IODevice);
}

//*************************************************************************************************************

FiffIO::FiffIO(QList<QIODevice*>& p_qlistIODevices)
{
    QList<QIODevice*>::iterator i;
    for(i = p_qlistIODevices.begin(); i != p_qlistIODevices.end(); ++i) {
        FiffIO::read((**i));
    }
}

//*************************************************************************************************************

bool FiffIO::setup_read(QIODevice& p_IODevice, FiffInfo& info, FiffDirTree& Tree, FiffDirTree& dirTree)
{
    //Open the file
    FiffStream::SPtr p_pStream(new FiffStream(&p_IODevice));
    QString t_sFileName = p_pStream->streamName();

    printf("Opening fiff data %s...\n",t_sFileName.toUtf8().constData());

    QList<FiffDirEntry> t_dirEntry;

    if(!p_pStream->open(Tree, t_dirEntry))
        return false;

    //Read the measurement info
    if(!p_pStream->read_meas_info(Tree, info, dirTree))
        return false;

    return true;
}

//*************************************************************************************************************
bool FiffIO::read(QIODevice& p_IODevice)
{
    //Read dirTree from fiff data (raw,evoked,fwds,cov)
    FiffInfo t_fiffInfo;
    FiffDirTree t_Tree;
    FiffDirTree t_dirTree;
    bool hasRaw=false,hasEvoked=false,hasFwds=false;

    FiffIO::setup_read(p_IODevice,t_fiffInfo,t_Tree,t_dirTree);
    p_IODevice.close(); //file can be closed, since IODevice is already read

    //Search dirTree for specific data types
    if(t_dirTree.has_kind(FIFFB_EVOKED))
        hasEvoked = true;
    if(t_dirTree.has_kind(FIFFB_RAW_DATA)) //this type might not yet be sufficient, (another is e.g. FIFFB_RAW_DATA)
        hasRaw = true;
    if(t_Tree.has_kind(FIFFB_MNE_FORWARD_SOLUTION))
        hasFwds = true;

    //Read all sort of types
    //raw data
    if(hasRaw) {
        QSharedPointer<FiffRawData> p_fiffRawData(new FiffRawData(p_IODevice));
        p_IODevice.close();

        //append to corresponding member qlist
        m_qlistRaw.append(p_fiffRawData);

        printf("Finished reading raw data!");
    }

    //evoked data + projections
    if(hasEvoked) {
        FiffEvokedSet p_fiffEvokedSet(p_IODevice);
        p_IODevice.close();

        //append to corresponding member qlist
        for(qint32 i=0; i < p_fiffEvokedSet.evoked.size(); ++i) {
            m_qlistEvoked.append(QSharedPointer<FiffEvoked>(&p_fiffEvokedSet.evoked[i]));
        }
    }

//    //forward solutions
//    if(hasFwds) {
//        MNEForwardSolution p_forwardSolution(p_IODevice);

//        //append to corresponding member qlist
//        m_qlistFwd.append(QSharedPointer<MNEForwardSolution>(&p_forwardSolution));
//    }

    //print summary
    //std::cout << *this << std::endl;

    return true;
}

//*************************************************************************************************************

bool FiffIO::write(QIODevice& p_IODevice, fiff_int_t type, fiff_int_t idx) {
    //ToDo: change QIODevice input to QString in order to output multiple files

    switch(type) {
        case FIFFB_RAW_DATA: {
            if(idx == -1) {
                for(qint32 i=0; i < m_qlistRaw.size(); ++i) {
                    //Alter filename in ascending numbering
                    QFile t_file(new QFile(&p_IODevice));
                    QString t_fname = t_file.fileName();
                    qint32 p = t_file.fileName().indexOf(".fif");
                    t_file.setFileName(t_file.fileName().insert(p,QString("-"+QString::number(i))));

                    std::cout << t_file.fileName().toUtf8().constData() << std::endl;

                    FiffIO::write_raw(p_IODevice,i);
                }
            }
            else {
                FiffIO::write_raw(p_IODevice,idx);
            }
            printf("Finished Writing!\n");
        }
//        case FIFFB_EVOKED:
            //ToDo: write evoked set to file
    }

    return true;

}

//*************************************************************************************************************

bool FiffIO::write_raw(QIODevice& p_IODevice, fiff_int_t idx) {

    MatrixXd cals;

//    std::cout << "Writing file " << QFile(&p_IODevice).fileName() << std::endl;
    FiffStream::SPtr outfid = Fiff::start_writing_raw(p_IODevice,this->m_qlistRaw[idx]->info,cals);

    //Setup reading parameters
    fiff_int_t from = m_qlistRaw[idx]->first_samp;
    fiff_int_t to = m_qlistRaw[idx]->last_samp;
    float quantum_sec = 10.0f;//read and write in 10 sec junks
    fiff_int_t quantum = ceil(quantum_sec*m_qlistRaw[idx]->info.sfreq);

    // To read the whole file at once
    //        quantum = to - from + 1;

    // Read and write all the data
    bool first_buffer = true;

    fiff_int_t first, last;
    MatrixXd data;
    MatrixXd times;

    for(first = from; first < to; first+=quantum) {
        last = first+quantum-1;
        if (last > to)
            last = to;

        if (!m_qlistRaw[idx]->read_raw_segment(data,times,first,last)) {
                printf("error during read_raw_segment\n");
                return -1;
        }

        printf("Writing...");
        if (first_buffer) {
           if (first > 0)
               outfid->write_int(FIFF_FIRST_SAMPLE,&first);
           first_buffer = false;
        }
        outfid->write_raw_buffer(data,cals);
        printf("[done]\n");
    }

    outfid->finish_writing_raw();

    return true;
}

//*************************************************************************************************************
