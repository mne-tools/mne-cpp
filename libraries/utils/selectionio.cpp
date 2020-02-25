//=============================================================================================================
/**
 * @file     selectionio.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @version  dev
 * @date     October, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the SelectionIO class
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "selectionio.h"

#include <iostream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFileInfo>
#include <QTextStream>
#include <QFile>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SelectionIO::SelectionIO()
{
}

//=============================================================================================================

bool SelectionIO::readMNESelFile(QString path, QMap<QString,QStringList> &selectionMap)
{
    //Open .sel file
    if(!path.contains(".sel"))
        return false;

    //clear the map first
    selectionMap.clear();

    QFile file(path);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug()<<"Error opening selection file";
        return false;
    }

    //Start reading from file
    QTextStream in(&file);

    while(!in.atEnd()) {
        QString line = in.readLine();

        if(line.contains("%") == false && line.contains(":") == true) //Skip commented areas in file
        {
            QStringList firstSplit = line.split(":");

            //Create new key
            QString key = firstSplit.at(0);

            QStringList secondSplit = firstSplit.at(1).split("|");

            //Delete last element if it is a blank character
            if(secondSplit.at(secondSplit.size()-1) == "")
                secondSplit.removeLast();

            //Add to map
            selectionMap.insertMulti(key, secondSplit);
        }
    }

    file.close();

    return true;
}

//=============================================================================================================

bool SelectionIO::readBrainstormMonFile(QString path, QMap<QString,QStringList> &selectionMap)
{
    //Open .sel file
    if(!path.contains(".mon"))
        return false;

    //clear the map first
    selectionMap.clear();

    QFile file(path);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug()<<"Error opening montage file";
        return false;
    }

    //Start reading from file
    QTextStream in(&file);
    QString groupName = in.readLine();
    QStringList channels;

    while(!in.atEnd()) {
        QString line = in.readLine();

        if(line.contains(":") == true) {
            QStringList secondSplit = line.split(":");
            QString key = secondSplit.at(0);
            channels.append(key);
        }
    }

    //Add to map
    selectionMap.insertMulti(groupName, channels);

    file.close();

    return true;
}

//=============================================================================================================

bool SelectionIO::writeMNESelFile(QString path, const QMap<QString,QStringList> &selectionMap)
{
    //Open .sel file
    if(!path.contains(".sel"))
        return false;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)){
        qDebug()<<"Error opening sel file for writing";
        return false;
    }

    //Write selections to file
    QTextStream out(&file);

    QMap<QString, QStringList>::const_iterator i = selectionMap.constBegin();
    while (i != selectionMap.constEnd()) {
        out << i.key() << ":";

        for(int u=0; u<i.value().size() ; u++)
            out << i.value().at(u) << "|";

        out << "\n" << "\n";

        ++i;
    }

    file.close();

    return true;
}

//=============================================================================================================

bool SelectionIO::writeBrainstormMonFiles(QString path, const QMap<QString,QStringList> &selectionMap)
{
    //Open .sel file
    if(!path.contains(".mon"))
        return false;

    QMapIterator<QString,QStringList> i(selectionMap);
    while (i.hasNext()) {
        i.next();

        QFileInfo fileInfo(path);

        QString newPath = QString("%1/%2.mon").arg(fileInfo.absolutePath()).arg(i.key());

        //std::cout<<newPath.toStdString()<<std::endl;

        QFile file(newPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)){
            qDebug()<<"Error opening mon file for writing";
            return false;
        }

        //Write selections to file
        QTextStream out(&file);

        out<<i.key()<<"\n";

        for(int u=0; u<i.value().size() ; u++)
            out << i.value().at(u) << " : " << i.value().at(u) << "\n";

        file.close();
    }

    return true;
}
