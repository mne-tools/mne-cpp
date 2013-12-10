//=============================================================================================================
/**
* @file     fiff_obj.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Florian Schlembach <florian.schlembach@tu-ilmenau.de>;
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
        FiffRawData p_fiffRawData(p_IODevice);
        p_IODevice.close();

        //append to corresponding member qlist
        m_qlistRaw.append(QSharedPointer<FiffRawData>(&p_fiffRawData));
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

    if(type == FIFFB_RAW_DATA) {
//        MatrixXd cals;

//        FiffStream::SPtr outfid = Fiff::start_writing_raw(p_IODevice,this->m_qlistRaw[0]->info, cals, picks);
//        //
//        //   Set up the reading parameters
//        //
//        fiff_int_t from = raw.first_samp;
//        fiff_int_t to = raw.last_samp;
//        float quantum_sec = 10.0f;//read and write in 10 sec junks
//        fiff_int_t quantum = ceil(quantum_sec*raw.info.sfreq);
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
//        MatrixXd data;
//        MatrixXd times;

//        for(first = from; first < to; first+=quantum)
//        {
//            last = first+quantum-1;
//            if (last > to)
//            {
//                last = to;
//            }

//            if (!raw.read_raw_segment(data,times,first,last,picks))
//            {
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

//        outfid->finish_writing_raw();

//        printf("Finished\n");
    }

    return true;

}

//*************************************************************************************************************
/* QObject must not be copied!

FiffIO::FiffIO(const FiffIO& p_FiffIO)
: m_qlistRaw(p_FiffIO.m_qlistRaw)
, m_qlistEvoked(p_FiffIO.m_qlistEvoked)
, m_qlistProj(p_FiffIO.m_qlistProj)
, m_qlistFwd(p_FiffIO.m_qlistFwd)
, m_qlistCov(p_FiffIO.m_qlistCov)
, m_qlistNMatrix(p_FiffIO.m_qlistNMatrix)
{

}
*/
//*************************************************************************************************************
