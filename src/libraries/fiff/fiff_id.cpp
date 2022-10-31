//=============================================================================================================
/**
 * @file     fiff_id.cpp
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
 * @brief    Definition of the FiffId Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_id.h"
#include "fiff_file.h"

#include <QNetworkInterface>
#include <QDateTime>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FiffId::FiffId()
: version(-1)
{
    machid[0] = -1;
    machid[1] = -1;
    time.secs = -1;
    time.usecs = -1;
}

//=============================================================================================================

FiffId::FiffId(const FiffId& p_FiffId)
: version(p_FiffId.version)
{
    machid[0] = p_FiffId.machid[0];
    machid[1] = p_FiffId.machid[1];
    time.secs = p_FiffId.time.secs;
    time.usecs = p_FiffId.time.usecs;
}

//=============================================================================================================

FiffId::~FiffId()
{
}

//=============================================================================================================

FiffId FiffId::new_file_id()
{
    FiffId id;
    id.version = FIFFC_VERSION;

    int fixed_id[2];
    get_machid(fixed_id);
    /*
     * Internet address in the first two words
     */
    id.machid[0] = fixed_id[0];
    id.machid[1] = fixed_id[1];
    /*
     * Time in the third and fourth words
     */
    /*
     * Time in the third and fourth words
     * Since practically no system gives times in
     * true micro seconds, the last three digits
     * are randomized to insure uniqueness.
     */
    {
        id.time.secs = QDateTime::currentMSecsSinceEpoch()/1000;
        id.time.usecs = rand() % 1000;
    }
    return id;
}

//=============================================================================================================

void FiffId::clear()
{
    version = -1;
    machid[0] = -1;
    machid[1] = -1;
    time.secs = -1;
    time.usecs = -1;
}

//=============================================================================================================

bool FiffId::get_machid(int *fixed_id)
{
    QList<QString> possibleHardwareAdresses;

    #ifndef WASMBUILD
    QList<QNetworkInterface> ifaces = QNetworkInterface::allInterfaces();

    fixed_id[0] = 0;
    fixed_id[1] = 0;
    if ( !ifaces.isEmpty() ) {
        for(int i = 0; i < ifaces.size(); ++i) {
            unsigned int flags = ifaces[i].flags();
            bool isLoopback = (bool)(flags & QNetworkInterface::IsLoopBack);
            bool isP2P = (bool)(flags & QNetworkInterface::IsPointToPoint);
            bool isRunning = (bool)(flags & QNetworkInterface::IsRunning);
            // If this interface isn't running, we don't care about it
            if ( !isRunning ) continue;
            // We only want valid interfaces that aren't loopback/virtual and not point to point
            if ( !ifaces[i].isValid() || isLoopback || isP2P ) continue;
            possibleHardwareAdresses << ifaces[i].hardwareAddress();
        }
        if (possibleHardwareAdresses.size() > 0) {
            // We take the first address as machine identifier
            QStringList hexPresentation = possibleHardwareAdresses[0].split(":");
            if(hexPresentation.size() == 6) {
                fixed_id[0] = QString(hexPresentation[0] + hexPresentation[1] + hexPresentation[2]).toInt(NULL,16);
                fixed_id[1] = QString(hexPresentation[3] + hexPresentation[4] + hexPresentation[5]).toInt(NULL,16);
                return true;
            }
        }
    }
    #endif

    return false;
}

//=============================================================================================================

void FiffId::print() const
{
    if(!isEmpty()) {
        printf ("\t%d.%d ",this->version>>16,this->version & 0xFFFF);
        printf ("0x%x%x ",this->machid[0],this->machid[1]);
        printf ("%d %d ",this->time.secs,this->time.usecs);
    }
}

//=============================================================================================================

QString FiffId::toMachidString() const
{
    QString strOut = QString("%1%2").arg(machid[0],8,16,QChar('0')).arg(machid[1],8,16,QChar('0'));

//    to do...
//    macid is 6 bytes of data->12 chars.
//    here macid is stored in two integers --> 8 bytes --> 16 chars.
//    some versions of sinuhe store the significant chars at the beginning of the 16 chars.
//    other versions sotre the at the end. I don't know on what it depends on.
//    clue 1: version 1.3 stores it at the beginning. (padding with 4 '0' chars at the end).
//    clue 2: version 1.2 stores it at the ending chars. (padding with 4 '0' chars at the beginning of the 16).
//    I've no idea if this behaviour is solid...
//    int thresholdMayorVersion(1);
//    int thresholdMinorVersion(2);

//    int thresholdVersionInt(static_cast<int>(thresholdMayorVersion*pow(2.,16))+thresholdMinorVersion);
//    if(version > thresholdVersionInt) //if this.version > 65538
//    {
//        strOut.chop(4);
//    } else
//    {
//        strOut.right(strOut.size()-4);
//    }

    int step=2;
    for(int i=step;i < strOut.size(); i+=step+1)
    {
        strOut.insert(i,QChar(':'));
    }

    return strOut.toUpper();
}

//=============================================================================================================

FiffId& FiffId::getDefault()
{
    static FiffId defaultFiffId;
    return defaultFiffId;
}
