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
// QT Includes
//=============================================================================================================

#include <QtWidgets>
#include <QtCore/QtPlugin>
#include <QDebug>

//*************************************************************************************************************
//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

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

/**
* Implements a client for communication with a fieldtrip bufer.
*
* @brief Fieldtrip Buffer Client Implementation
*/
class FtBuffClient
{
public:
    /**
     * Default Constructor
     */
    FtBuffClient();

    //=========================================================================================================
    /**
     * @brief FtBuffClient - Constructor that set address for buffer
     * @param addr [in] (address) - tells the client the address and port of the buffer.
     */
    FtBuffClient(char* addr);

    //=========================================================================================================
    /**
     * @brief getDataExample - Made to run with FT buffer examples: sine2ft and buffer.
     */
    void getDataExample();

    //=========================================================================================================
    /**
     * @brief getData - tries to get data from buffer
     */
    void getData();

    //=========================================================================================================
    /**
     * @brief getAddress - returns address the client has stored.
     */
    QString getAddress();

    //=========================================================================================================
    /**
     * @brief stopConnection - stops connecton with buffer if one is currently open
     */
    bool stopConnection();

    //=========================================================================================================
    /**
     * @brief startConnection - starts connection with buffer if one does not already exist
     */
    bool startConnection();


//protected:
//signals:

    //void newDataAvailable(const Eigen::MatrixXd &matData);

private:

    /**
     * @brief readHeader - Attempts to get and read header from buffer. This gives us info about the data in the buffer.
     * @return Returns whether header was succesffully retrieved and read.
     */
    bool readHeader();

    //=========================================================================================================
    /**
     * @brief idleCall - gets called repetedly to get new data from buffer.
     */
    void idleCall();

    //=========================================================================================================
    template<typename T>
    void convertToFloat(float *dest, const void *src, unsigned int nsamp, unsigned int nchans);

    //=========================================================================================================
    /**
     * @brief isConnected - returns whether a connection is open with a buffer.
     * @return true - connection open / false - no connection
     */
    bool isConnected();

    //=========================================================================================================

    void outputSamples(int size, const float* data);

    //=========================================================================================================

    int                 numChannels;
    uint                numSamples;

    FtConnection        ftCon;
    const char*         addrField;

    SimpleStorage       rawStore, floatStore;

    bool                useHighpass;
    bool                useLowpass;

    //TODO: remove this, it's from the viewer.cc GUI, only here to make porting code easier
    char**              labels;
    int*                colorTable;

    MultiChannelFilter<float,float> *hpFilter = NULL;
    MultiChannelFilter<float,float> *lpFilter = NULL;

    float               *data;

    //These are from the OnlineDataDisplay class that updates the viewer.cc data
    int                 nChans;
    int                 nSamp;

    int                 numTotal, pos;
    int                 yPos, ySpace;
    float               yScale;
    int                 height;
    int                 skipped;

    bool                newData;



};

#endif // FTBUFFCLIENT_H
