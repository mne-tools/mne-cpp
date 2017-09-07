//=============================================================================================================
/**
* @file     mne.cpp
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
* @brief     implementation of the MNE Wrapper Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne.h"
#include <fiff/fiff.h>
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

bool MNE::read_events(QIODevice &p_IODevice, MatrixXi& eventlist)
{
    //
    // Open file
    //
    FiffStream::SPtr t_pStream(new FiffStream(&p_IODevice));

    if(!t_pStream->open()) {
        return false;
    }

    //
    //   Find the desired block
    //
    QList<FiffDirNode::SPtr> events = t_pStream->dirtree()->dir_tree_find(FIFFB_MNE_EVENTS);

    if (events.size() == 0)
    {
        printf("Could not find event data\n");
        return false;
    }

    qint32 k, nelem;
    fiff_int_t kind, pos;
    FiffTag::SPtr t_pTag;
    quint32* serial_eventlist_uint = NULL;
    qint32* serial_eventlist_int = NULL;

    for(k = 0; k < events[0]->nent(); ++k)
    {
        kind = events[0]->dir[k]->kind;
        pos  = events[0]->dir[k]->pos;
        if (kind == FIFF_MNE_EVENT_LIST)
        {
            t_pStream->read_tag(t_pTag,pos);
            if(t_pTag->type == FIFFT_UINT)
            {
                serial_eventlist_uint = t_pTag->toUnsignedInt();
                nelem = t_pTag->size()/4;
            }

            if(t_pTag->type == FIFFT_INT)
            {
                serial_eventlist_int = t_pTag->toInt();
                nelem = t_pTag->size()/4;
            }

            break;
        }
    }

    if(serial_eventlist_uint == NULL && serial_eventlist_int == NULL)
    {
        printf("Could not find any events\n");
        return false;
    }
    else
    {
        eventlist.resize(nelem/3,3);
        if(serial_eventlist_uint != NULL)
        {
            for(k = 0; k < nelem/3; ++k)
            {
                eventlist(k,0) = serial_eventlist_uint[k*3];
                eventlist(k,1) = serial_eventlist_uint[k*3+1];
                eventlist(k,2) = serial_eventlist_uint[k*3+2];
            }
        }

        if(serial_eventlist_int != NULL)
        {
            for(k = 0; k < nelem/3; ++k)
            {
                eventlist(k,0) = serial_eventlist_int[k*3];
                eventlist(k,1) = serial_eventlist_int[k*3+1];
                eventlist(k,2) = serial_eventlist_int[k*3+2];
            }
        }
    }

    return true;
}


//*************************************************************************************************************
