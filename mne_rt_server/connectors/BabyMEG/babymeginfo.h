//=============================================================================================================
/**
* @file     babymeginfo.h
* @author   Limin Sun <liminsun@nmr.mgh.harvard.edu>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     April, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Limin Sun, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief     implementation of the BabyMEGInfo Class.
*
*/

#ifndef BABYMEGINFO_H
#define BABYMEGINFO_H

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================
#include <QObject>
#include <QQueue>
#include <QtCore>

//=============================================================================================================
/**
* DECLARE CLASS BabyMEGInfo
*
* @brief The BabyMEGClient class provides a TCP/IP communication between Qt and Labview.
*/
class BabyMEGInfo
{
public:
    //=========================================================================================================
    /**
    * Constructs a BabyMEGInfo class.
    */
    BabyMEGInfo();
public:
    int chnNum;
    int dataLength;

    //BB_QUEUE
    QQueue<QByteArray> g_queue;
    int g_maxlen;
    QMutex g_mutex;
    QWaitCondition g_queueNotFull;
    QWaitCondition g_queueNotEmpty;

public:
    //=========================================================================================================
    /**
    * Parse the information about header information
    *
    * @param[in] cmdstr - QByteArray contains the header information.
    */
    void MGH_LM_Parse_Para(QByteArray cmdstr);
    //=========================================================================================================
    /**
    * Put data block into a queue
    *
    * @param[in] DataIn - Input Data Block (QByteArray).
    */
    void EnQueue(QByteArray DataIn);
    //=========================================================================================================
    /**
    * Get data block from a queue
    *
    * @param[in] void
    * @param[out] Output Data Block (QByteArray)
    */
    QByteArray DeQueue();

};

#endif // BABYMEGINFO_H
