//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file fiff_id.cpp
 * @since October 2012
 * @brief Implementation of @ref FiffId: streaming of the 20-byte FIFF identifier record (version + machid + creation time).
 *
 * The @c machid is filled from the host MAC address when generating new
 * IDs so written files keep parity with the Neuromag acquisition stack
 * and stay lineage-traceable.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_id.h"
#include "fiff_file.h"

#ifndef __EMSCRIPTEN__
#include <QNetworkInterface>
#endif
#include <QDateTime>

#include <ctime>
#include <QDebug>

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

    #ifndef __EMSCRIPTEN__
    QList<QNetworkInterface> ifaces = QNetworkInterface::allInterfaces();

    fixed_id[0] = 0;
    fixed_id[1] = 0;
    if ( !ifaces.isEmpty() ) {
        for(int i = 0; i < ifaces.size(); ++i) {
            unsigned int flags = ifaces[i].flags();
            bool isLoopback = static_cast<bool>(flags & QNetworkInterface::IsLoopBack);
            bool isP2P = static_cast<bool>(flags & QNetworkInterface::IsPointToPoint);
            bool isRunning = static_cast<bool>(flags & QNetworkInterface::IsRunning);
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
                fixed_id[0] = QString(hexPresentation[0] + hexPresentation[1] + hexPresentation[2]).toInt(nullptr,16);
                fixed_id[1] = QString(hexPresentation[3] + hexPresentation[4] + hexPresentation[5]).toInt(nullptr,16);
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
        qInfo("\t%d.%d 0x%x%x %d %d\n",this->version>>16,this->version & 0xFFFF,this->machid[0],this->machid[1],this->time.secs,this->time.usecs);
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

//=============================================================================================================

QString FiffId::toString() const
{
    time_t secs = time.secs;
    struct tm *ltime = localtime(&secs);
    char timebuf[100];
    strftime(timebuf, sizeof(timebuf), "%c", ltime);
    return QString("%1.%2 0x%3%4 %5")
        .arg(version >> 16)
        .arg(version & 0xFFFF)
        .arg(machid[0], 0, 16)
        .arg(machid[1], 0, 16)
        .arg(QString::fromLatin1(timebuf));
}
