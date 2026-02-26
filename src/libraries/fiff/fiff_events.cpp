//=============================================================================================================
/**
 * @file     fiff_events.cpp
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
 * @brief    Definition of the FiffEvents Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_events.h"
#include "fiff_raw_data.h"
#include "fiff_stream.h"
#include "fiff_dir_node.h"
#include "fiff_tag.h"
#include "fiff_constants.h"
#include "fiff_file.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QDebug>
#include <QTextStream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

bool FiffEvents::read(QString t_sEventName,
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
        if(!FiffEvents::read_from_fif(t_EventFile, events)) {
            printf("Error while read events.\n");
            return false;
        }
        printf("Events read from %s\n",t_sEventName.toUtf8().constData());
    } else {
        // Binary file
        if (t_sEventName.contains(".fif")) {
            t_EventFile.setFileName(t_sEventName);
            if(!FiffEvents::read_from_fif(t_EventFile, events)) {
                printf("Error while read events.\n");
                return false;
            }
            printf("Binary event file %s read\n",t_sEventName.toUtf8().constData());
        } else if(t_sEventName.contains(".eve")){

        } else {
            // Text file
            printf("Text file %s is not supported jet.\n",t_sEventName.toUtf8().constData());
        }
    }

    return true;
}

//=============================================================================================================

bool FiffEvents::read_from_fif(QIODevice &p_IODevice,
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

bool FiffEvents::read_from_ascii(QIODevice &p_IODevice,
                                 MatrixXi& eventlist)
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

//=============================================================================================================

bool FiffEvents::write_to_fif(QIODevice &p_IODevice,
                              const MatrixXi& eventlist)
{
    if (eventlist.rows() == 0 || eventlist.cols() < 3)
        return false;

    FiffStream::SPtr pStream = FiffStream::start_file(p_IODevice);
    if (!pStream)
        return false;

    pStream->start_block(FIFFB_MNE_EVENTS);
    pStream->write_int(FIFF_MNE_EVENT_LIST, eventlist.data(), eventlist.rows() * 3);
    pStream->end_block(FIFFB_MNE_EVENTS);
    pStream->end_file();

    return true;
}

//=============================================================================================================

bool FiffEvents::write_to_ascii(QIODevice &p_IODevice,
                                const MatrixXi& eventlist,
                                float sfreq)
{
    if (!p_IODevice.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&p_IODevice);
    for (int k = 0; k < eventlist.rows(); ++k) {
        int sample = eventlist(k, 0);
        int before = (eventlist.cols() > 1) ? eventlist(k, 1) : 0;
        int after  = (eventlist.cols() > 2) ? eventlist(k, 2) : 0;
        float time = (sfreq > 0.0f) ? static_cast<float>(sample) / sfreq : 0.0f;
        out << QString("%1 %2 %3 %4\n")
               .arg(sample, 6)
               .arg(time, -10, 'f', 3)
               .arg(before, 3)
               .arg(after, 3);
    }

    p_IODevice.close();
    return true;
}

//=============================================================================================================

MatrixXi FiffEvents::detect_from_raw(const FiffRawData &raw,
                                     const QString &triggerCh,
                                     unsigned int triggerMask,
                                     bool leadingEdge)
{
    QString stimCh = triggerCh.isEmpty() ? QString("STI 014") : triggerCh;

    // Find trigger channel index
    int triggerChIdx = -1;
    for (int k = 0; k < raw.info.ch_names.size(); ++k) {
        if (raw.info.ch_names[k] == stimCh) {
            triggerChIdx = k;
            break;
        }
    }
    if (triggerChIdx < 0) {
        qWarning() << "[FiffEvents::detect_from_raw] Trigger channel" << stimCh << "not found.";
        return MatrixXi(0, 3);
    }

    // Read trigger channel data
    MatrixXd data;
    MatrixXd times;
    if (!raw.read_raw_segment(data, times, raw.first_samp, raw.last_samp,
                              RowVectorXi::LinSpaced(1, triggerChIdx, triggerChIdx))) {
        qWarning() << "[FiffEvents::detect_from_raw] Could not read trigger channel data.";
        return MatrixXi(0, 3);
    }

    RowVectorXd trigData = data.row(0);
    int nSamples = static_cast<int>(trigData.cols());

    // Detect flanks
    QList<int> eventSamples;
    QList<int> eventBefore;
    QList<int> eventAfter;

    int prevVal = static_cast<int>(trigData(0)) & triggerMask;
    for (int s = 1; s < nSamples; ++s) {
        int curVal = static_cast<int>(trigData(s)) & triggerMask;
        if (curVal != prevVal) {
            if (!leadingEdge || (leadingEdge && prevVal == 0 && curVal != 0)) {
                eventSamples.append(static_cast<int>(raw.first_samp) + s);
                eventBefore.append(prevVal);
                eventAfter.append(curVal);
            }
        }
        prevVal = curVal;
    }

    int nEvents = eventSamples.size();
    MatrixXi events(nEvents, 3);
    for (int k = 0; k < nEvents; ++k) {
        events(k, 0) = eventSamples[k];
        events(k, 1) = eventBefore[k];
        events(k, 2) = eventAfter[k];
    }

    return events;
}
