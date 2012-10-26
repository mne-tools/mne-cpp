//=============================================================================================================
/**
* @file     mne.cpp
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
* @brief    Contains the implementation of the MNE Wrapper Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne.h"
#include "../fiff/fiff.h"
#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

bool MNE::read_events(QIODevice* p_pIODevice, MatrixXi& eventlist)
{
    //
    // Open file
    //
    FiffStream* t_pFile = new FiffStream(p_pIODevice);
    FiffDirTree* t_pTree = NULL;
    QList<FiffDirEntry>* t_pDir = NULL;

    if(!t_pFile->open(t_pTree, t_pDir))
    {
        if(t_pTree)
            delete t_pTree;

        if(t_pDir)
            delete t_pDir;

        return false;
    }

    //
    //   Find the desired block
    //
    QList<FiffDirTree*> events = t_pTree->dir_tree_find(FIFFB_MNE_EVENTS);

    if (events.size() == 0)
    {
        printf("Could not find event data\n");
        delete t_pFile;
        delete t_pTree;
        delete t_pDir;
        return false;
    }

    qint32 k, nelem;
    fiff_int_t kind, pos;
    FiffTag* t_pTag = NULL;
    quint32* serial_eventlist = NULL;
    for(k = 0; k < events[0]->nent; ++k)
    {
        kind = events[0]->dir[k].kind;
        pos  = events[0]->dir[k].pos;
        if (kind == FIFF_MNE_EVENT_LIST)
        {
            FiffTag::read_tag(t_pFile,t_pTag,pos);
            if(t_pTag->type == FIFFT_UINT)
            {
                serial_eventlist = t_pTag->toUnsignedInt();
                nelem = t_pTag->size()/4;
            }
            break;
        }
    }

    if(serial_eventlist == NULL)
    {
        delete t_pFile;
        delete t_pTree;
        delete t_pDir;
        delete t_pTag;
        printf("Could not find any events\n");
        return false;
    }
    else
    {
        eventlist.resize(nelem/3,3);
        for(k = 0; k < nelem/3; ++k)
        {
            eventlist(k,0) = serial_eventlist[k*3];
            eventlist(k,1) = serial_eventlist[k*3+1];
            eventlist(k,2) = serial_eventlist[k*3+2];
        }
    }

    delete t_pFile;
    delete t_pTree;
    delete t_pDir;
    delete t_pTag;
    return true;
}


//*************************************************************************************************************
