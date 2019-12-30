//=============================================================================================================
/**
* @file     ftbuffclient.h
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
* @brief    FtBuffClient class declaration.
*
*/

#ifndef FTBUFFCLIENT_H
#define FTBUFFCLIENT_H

//*************************************************************************************************************
//=============================================================================================================
// Includes
//=============================================================================================================

#include <FtBuffer.h>
#include <MultiChannelFilter.h>
#include <buffer.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

//*************************************************************************************************************
//=============================================================================================================
// DEFINITIONS
//=============================================================================================================

#define HPFILTORD 2
#define HPFREQ  4.0
#define LPFILTORD 7
#define LPFREQ 70.0
#define HIDDEN  16


class FtBuffClient
{

public:
    FtBuffClient();
    FtBuffClient(char* addr);

    void getDataExample();

private:

    bool readHeader();
    void idleCall();

    template<typename T>
    void convertToFloat(float *dest, const void *src, unsigned int nsamp, unsigned int nchans);

    void stopConnection();
    void startConnection();

    bool isConnected();


    int numChannels = 0;
    uint numSamples = 0;

    FtConnection ftCon;
    const char* addrField;


    MultiChannelFilter<float,float> *hpFilter = NULL;
    MultiChannelFilter<float,float> *lpFilter = NULL;

    SimpleStorage rawStore, floatStore;

    bool useHighpass = false;
    bool useLowpass = false;

    //TODO: remove this, it's from the viewer.cc GUI, only here to make porting code easier
    char **labels;
    int *colorTable;

};

#endif // FTBUFFCLIENT_H
