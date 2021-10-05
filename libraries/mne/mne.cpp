//=============================================================================================================
/**
 * @file     mne.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 *           Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Matti Hamalainen, Christoph Dinh, Gabriel Motta. All rights reserved.
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
 * @brief     Definition of the MNE Wrapper Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne.h"
#include <fiff/fiff.h>
#include <iostream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace Eigen;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

bool MNE::read_events(QString t_sEventName,
                      QString t_fileRawName,
                      MatrixXi& events)
{
    QFile t_EventFile;
    qint32 p;

    if (t_sEventName.isEmpty()) {
        p = t_fileRawName.indexOf(".fif");
        if (p > 0) {
            t_sEventName = t_fileRawName.replace(p, 4, "-eve.fif");
        } else {
            printf("Raw file name does not end properly\n");
            return 0;
        }

        t_EventFile.setFileName(t_sEventName);
        if(!MNE::read_events_from_fif(t_EventFile, events)) {
            printf("Error while read events.\n");
            return false;
        }
        printf("Events read from %s\n",t_sEventName.toUtf8().constData());
    } else {
        // Binary file
        if (t_sEventName.contains(".fif")) {
            t_EventFile.setFileName(t_sEventName);
            if(!MNE::read_events_from_fif(t_EventFile, events)) {
                printf("Error while read events.\n");
                return false;
            }
            printf("Binary event file %s read\n",t_sEventName.toUtf8().constData());
        } else if(t_sEventName.contains(".eve")){

        } else {
            // Text file
            printf("Text file %s is not supported jet.\n",t_sEventName.toUtf8().constData());
//            try
//                events = load(eventname);
//            catch
//                error(me,mne_omit_first_line(lasterr));
//            end
//            if size(events,1) < 1
//                error(me,'No data in the event file');
//            end
//            //
//            //   Convert time to samples if sample number is negative
//            //
//            for p = 1:size(events,1)
//                if events(p,1) < 0
//                    events(p,1) = events(p,2)*raw.info.sfreq;
//                end
//            end
//            //
//            //    Select the columns of interest (convert to integers)
//            //
//            events = int32(events(:,[1 3 4]));
//            //
//            //    New format?
//            //
//            if events(1,2) == 0 && events(1,3) == 0
//                fprintf(1,'The text event file %s is in the new format\n',eventname);
//                if events(1,1) ~= raw.first_samp
//                    error(me,'This new format event file is not compatible with the raw data');
//                end
//            else
//                fprintf(1,'The text event file %s is in the old format\n',eventname);
//                //
//                //   Offset with first sample
//                //
//                events(:,1) = events(:,1) + raw.first_samp;
//            end
        }
    }

    return true;
}

//=============================================================================================================

bool MNE::read_events_from_fif(QIODevice &p_IODevice,
                               MatrixXi& eventlist)
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

//=============================================================================================================

void MNE::setup_compensators(FiffRawData& raw,
                             fiff_int_t dest_comp,
                             bool keep_comp)
{
    // Set up projection
    if (raw.info.projs.size() == 0) {
        printf("No projector specified for these data\n");
    } else {
        // Activate the projection items
        for (qint32 k = 0; k < raw.info.projs.size(); ++k) {
            raw.info.projs[k].active = true;
        }

        printf("%d projection items activated\n",raw.info.projs.size());
        // Create the projector
//        fiff_int_t nproj = MNE::make_projector_info(raw.info, raw.proj); Using the member function instead
        fiff_int_t nproj = raw.info.make_projector(raw.proj);

        if (nproj == 0)  {
            printf("The projection vectors do not apply to these channels\n");
        } else {
            printf("Created an SSP operator (subspace dimension = %d)\n",nproj);
        }
    }

    // Set up the CTF compensator
//    qint32 current_comp = MNE::get_current_comp(raw.info);
    qint32 current_comp = raw.info.get_current_comp();
    if (current_comp > 0)
        printf("Current compensation grade : %d\n",current_comp);

    if (keep_comp)
        dest_comp = current_comp;

    if (current_comp != dest_comp)
    {
        qDebug() << "This part needs to be debugged";
        if(MNE::make_compensator(raw.info, current_comp, dest_comp, raw.comp))
        {
//            raw.info.chs = MNE::set_current_comp(raw.info.chs,dest_comp);
            raw.info.set_current_comp(dest_comp);
            printf("Appropriate compensator added to change to grade %d.\n",dest_comp);
        }
        else
        {
            printf("Could not make the compensator\n");
            return;
        }
    }
}

//=============================================================================================================

bool MNE::read_events_from_ascii(QIODevice &p_IODevice,
                                 Eigen::MatrixXi& eventlist)
{
    if (!p_IODevice.open(QIODevice::ReadOnly | QIODevice::Text)){
        return false;
    }
    QTextStream textStream(&p_IODevice);

    QList<int> simpleList;

    while(!textStream.atEnd()){
        int iSample;
        textStream >> iSample;
        simpleList.append(iSample);
        textStream.readLine();
        qDebug() << "Added event:" << iSample;
    }

    eventlist.resize(simpleList.size(), 1);

    for(int i = 0; i < simpleList.size(); i++){
        eventlist(i,0) = simpleList[i];
    }
    return true;
}
