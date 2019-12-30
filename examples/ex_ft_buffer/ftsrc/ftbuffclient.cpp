//=============================================================================================================
/**
* @file     ftbuffclient.cpp
* @author   Gabriel B. Motta <gbmotta@mgh.harvard.edu
*           Stefan Klanke
* @version  1.0
* @date     December, 2019
*
* @section  LICENSE
*
* Copyright (C) 2019, Stefan Klanke and Gabriel B. Motta. All rights reserved.
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
* Based on viewer.cc example from ftbuffer reference implementation, under the GNU GENERAL PUBLIC LICENSE Version 2:
*
* Copyright (C) 2010, Stefan Klanke
* Donders Institute for Donders Institute for Brain, Cognition and Behaviour,
* Centre for Cognitive Neuroimaging, Radboud University Nijmegen,
* Kapittelweg 29, 6525 EN Nijmegen, The Netherlands
*
* @brief    FtBuffClient class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ftbuffclient.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QTest>

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FtBuffClient::FtBuffClient() {
    addrField = "localhost:1972";
}

//*************************************************************************************************************

FtBuffClient::FtBuffClient(char* addr) {
    addrField = addr;
}

//*************************************************************************************************************

void FtBuffClient::getDataExample() {

    qDebug() << "================ FieldTrip Buffer Example ================";
    qDebug() << "Ensure the ft buffer source files are present and their respective 'make' files have been called.";

    while(true) {

        //Starts connection with ftCon on localhost:1972
        while(!this->isConnected()) {
            this->startConnection();
            QTest::qSleep(1000);
        }

        //Handle requests, responses, incoming data after connection is made
        this->idleCall();
        QTest::qSleep(10);
    }
}

//*************************************************************************************************************

bool FtBuffClient::readHeader() {

    //c++ wrapper classes for ftbuffer implementation
    //handles connections, requests, and storage of incoming data
    qDebug() << "Creating request/response handlers...";
    SimpleStorage chunkBuffer;
    headerdef_t header_def;
    FtBufferRequest request;
    FtBufferResponse response;


    //set request command to GET_HDR, other member variables to approprit values
    qDebug() << "Preparing header...";
    request.prepGetHeader();

    qDebug() << "Attempting TCP connection...";
    //Attempt to establish TCP connection
    if (tcprequest(ftCon.getSocket(), request.out(), response.in()) < 0) {
        qDebug() << "Error in communication - check buffer server";
        ftCon.disconnect();
        numChannels = 0;
        return false;
    }

    //Attempt to revieve and read header
    qDebug() << "Attempting to retrieve header...";
    if (!response.checkGetHeader(header_def, &chunkBuffer)) {
        qDebug() << "Could not read header.";
        return false;
    }

    //Updating channel and sample info
    numChannels = header_def.nchans;
    numSamples = header_def.nsamples;

    //from viewer.cc, only here temporarily to make porting easier.
    labels = (char **) calloc(numChannels, sizeof(char *));
    colorTable = (int *) calloc(numChannels, sizeof(int));


    const ft_chunk_t *cnc = find_chunk(chunkBuffer.data(), 0, chunkBuffer.size(), FT_CHUNK_CHANNEL_NAMES);
    if (cnc == NULL) {
        printf("No channel names found\n");
        for (int n=0;n<numChannels;n++) {
            labels[n] = (char *) malloc(8);
            snprintf(labels[n],7,"#%i",n+1);
        }
    } else {
        const char *s = (const char *) cnc->data;
        for (int n=0;n<numChannels;n++) {
            int ln = strlen(s);
            if (ln==0) {
                labels[n] = (char *) malloc(8);
                snprintf(labels[n],7,"#%i",n+1);
            } else {
                labels[n] = strdup(s);
            }
            s+=ln+1;
        }
    }

    if (hpFilter != NULL) {
        delete hpFilter;
        hpFilter = NULL;
    }
    if (lpFilter != NULL) {
        delete lpFilter;
        lpFilter = NULL;
    }

    hpFilter = new MultiChannelFilter<float,float>(numChannels, HPFILTORD);
    hpFilter->setButterHP(HPFREQ/header_def.fsample);
    lpFilter = new MultiChannelFilter<float,float>(numChannels, LPFILTORD);
    lpFilter->setButterLP(LPFREQ/header_def.fsample);
    return true;
}

//*************************************************************************************************************

template<typename T>
void FtBuffClient::convertToFloat(float *dest, const void *src, unsigned int nsamp, unsigned int nchans) {
    const T *srcT = static_cast<const T *>(src);
    for (unsigned int j=0;j<nsamp;j++) {
        for (unsigned int i=0;i<nchans;i++) {
            dest[i] = (float) srcT[i];
        }
        dest += nchans;
        srcT += nchans;
    }
}

//*************************************************************************************************************

//Checks if there is an open connection, and if so, closes it
void FtBuffClient::stopConnection() {
    if (ftCon.isOpen()) {
        qDebug() << "Disconnecting...";
        ftCon.disconnect();
        qDebug() << "Disconnected.";
    } else {
        qDebug() << "Not currently connected";
    }
}

//*************************************************************************************************************

//Checks if there is no open connection, and if so, opens one
void FtBuffClient::startConnection() {
    if (!ftCon.isOpen()) {
        qDebug() << "Connecting...";
        if (ftCon.connect(addrField)){
            qDebug() << "Connected to" << addrField;
        } else {
            qDebug() << "Unable to connect: Connection failed";
        }
    } else {
        qDebug() << "Unable to connect: Already connected";
    }
}

//*************************************************************************************************************

//gets called constantly and receives data
void FtBuffClient::idleCall() {

    datadef_t ddef;
    FtBufferRequest request;
    FtBufferResponse response;
    unsigned int newSamples, newEvents;

    if (!ftCon.isOpen()) return;

    if (numChannels == 0) {
        if (!readHeader()) {
            QTest::qSleep(50);
            return;
        }
    }

    request.prepWaitData(numSamples, 0xFFFFFFFF, 40);

    if (tcprequest(ftCon.getSocket(), request.out(), response.in()) < 0) {
        ftCon.disconnect();
        qDebug() << "Error in communication. Buffer server aborted??";
        return;
    }
    if (!response.checkWait(newSamples, newEvents)) {
        ftCon.disconnect();
        qDebug() << "Error in received packet - disconnecting...";
        return;
    }

    if (newSamples == numSamples) return; // nothing new
    if (newSamples < numSamples) {
        // oops ? do we have a new header?
        if (!readHeader()) return;
        if (numSamples == 0) return; // no data yet
        if (numSamples > 1024 || numChannels > 512) {
            // "lots" of data already in the buffer
            // -> don't do anything with that data
            //    continue next idleCall
            return;
        }
        // read data from the start of the buffer up to newSamples right away
        newSamples = numSamples;
        numSamples = 0;
    }

    request.prepGetData(numSamples, newSamples-1);

    if (tcprequest(ftCon.getSocket(), request.out(), response.in()) < 0) {
        ftCon.disconnect();
        qDebug() << "Error in communication. Buffer server aborted??";
        return;
    }
    if (!response.checkGetData(ddef, &rawStore)) {
        ftCon.disconnect();
        qDebug() << "Error in received packet - disconnecting...";
        return;
    }

    floatStore.resize(sizeof(float) * ddef.nsamples * ddef.nchans);

    float *fdata = (float *) floatStore.data();

    switch(ddef.data_type) {
        case DATATYPE_UINT8:
            convertToFloat<uint8_t>(fdata, rawStore.data(), ddef.nsamples, ddef.nchans);
            break;
        case DATATYPE_INT8:
            convertToFloat<int8_t>(fdata, rawStore.data(), ddef.nsamples, ddef.nchans);
            break;
        case DATATYPE_UINT16:
            convertToFloat<uint16_t>(fdata, rawStore.data(), ddef.nsamples, ddef.nchans);
            break;
        case DATATYPE_INT16:
            convertToFloat<int16_t>(fdata, rawStore.data(), ddef.nsamples, ddef.nchans);
            break;
        case DATATYPE_UINT32:
            convertToFloat<uint32_t>(fdata, rawStore.data(), ddef.nsamples, ddef.nchans);
            break;
        case DATATYPE_INT32:
            convertToFloat<int32_t>(fdata, rawStore.data(), ddef.nsamples, ddef.nchans);
            break;
        case DATATYPE_UINT64:
            convertToFloat<uint64_t>(fdata, rawStore.data(), ddef.nsamples, ddef.nchans);
            break;
        case DATATYPE_INT64:
            convertToFloat<int64_t>(fdata, rawStore.data(), ddef.nsamples, ddef.nchans);
            break;
        case DATATYPE_FLOAT32:
            convertToFloat<float>(fdata, rawStore.data(), ddef.nsamples, ddef.nchans);
            break;
        case DATATYPE_FLOAT64:
            convertToFloat<double>(fdata, rawStore.data(), ddef.nsamples, ddef.nchans);
            break;
    }

    if (useHighpass) {
        hpFilter->process(ddef.nsamples, fdata, fdata); // in place
    }
    if (useLowpass) {
        lpFilter->process(ddef.nsamples, fdata, fdata); // in place
    }

    numSamples = newSamples;
}

//*************************************************************************************************************

bool FtBuffClient::isConnected() {
    return ftCon.isOpen();
}
