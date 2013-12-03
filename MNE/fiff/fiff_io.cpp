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

//FiffIO::FiffIO(QList<QIODevice>& p_qlistIODevices)
//{
//    // execute read method..., iterate through QList
//}

//*************************************************************************************************************

bool FiffIO::setup_read(QIODevice& p_IODevice, FiffInfo& info, FiffDirTree& dirTree)
{
    //Open the file
    FiffStream::SPtr p_pStream(new FiffStream(&p_IODevice));
    QString t_sFileName = p_pStream->streamName();

    printf("Opening fiff data %s...\n",t_sFileName.toUtf8().constData());

    FiffDirTree t_Tree;
    QList<FiffDirEntry> t_Dir;

    if(!p_pStream->open(t_Tree, t_Dir))
        return false;

    //Read the measurement info
    if(!p_pStream->read_meas_info(t_Tree, info, dirTree))
        return false;

    return true;
}

//*************************************************************************************************************
bool FiffIO::read(QIODevice& p_IODevice)
{
    //Search dirTree of fiff data (raw,evoked,fwds,cov)
    FiffInfo t_fiffInfo;
    FiffDirTree t_dirTree;
    bool hasRaw=false,hasEvoked=false,hasFwds=false,hasCov=false;

    FiffIO::setup_read(p_IODevice,t_fiffInfo,t_dirTree);
    p_IODevice.close(); //file can be closed, since IODevice is already read

    /*
    if(t_dirTree.dir_tree_find(FIFFB_RAW_DATA).size()>0 || t_dirTree.dir_tree_find(FIFFB_CONTINUOUS_DATA).size()>0) { //ToDo: these constraint might not yet be sufficient
        hasRaw = true;*/
    if(t_dirTree.has_kind(101))
        hasEvoked = true;
    if(t_dirTree.has_kind(FIFFB_RAW_DATA)) //this type might not yet be sufficient, (another is e.g. FIFFB_RAW_DATA)
        hasRaw = true;

    //Read all sort of types
    //raw data
    FiffRawData p_fiffRawData(p_IODevice);
    p_IODevice.close();


    //evoked data + projections
    FiffEvokedSet p_fiffEvokedSet(p_IODevice);
    p_IODevice.close();

    //forward solutions
    MNEForwardSolution p_forwardSolution(p_IODevice);

    //cov
    FiffCov p_fiffCov(p_IODevice);

    //append everything to member qlists
    //raw
    if(!p_fiffRawData.rawdir.isEmpty()) {
        m_qlistRaw.append(QSharedPointer<FiffRawData>(&p_fiffRawData));
    }

    //evoked
    for(qint32 i=0; i < p_fiffEvokedSet.evoked.size(); ++i) {
        m_qlistEvoked.append(QSharedPointer<FiffEvoked>(&p_fiffEvokedSet.evoked[i]));
    }

    //forward solutions
    m_qlistFwd.append(QSharedPointer<MNEForwardSolution>(&p_forwardSolution));

    //cov
    m_qlistCov.append(QSharedPointer<FiffCov>(&p_fiffCov));

    //output summary
    std::cout << "\n---------------------- Fiff data read summary ---------------------- " << std::endl;
    std::cout << m_qlistRaw.size() << " raw data sets found! " << std::endl;
    std::cout << m_qlistEvoked.size() << " evoked sets found!" << std::endl;
    std::cout << m_qlistFwd[0]->source_ori << " forward solutions found!" << std::endl;
    std::cout << m_qlistCov.size() << " covariances found!" << std::endl;

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
