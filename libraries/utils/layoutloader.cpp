//=============================================================================================================
/**
* @file     layoutloader.cpp
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     September, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Definition of the LayoutLoader class
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "layoutloader.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QIODevice>
#include <QTextStream>
#include <QFile>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

LayoutLoader::LayoutLoader()
{
}


//*************************************************************************************************************

bool LayoutLoader::readAsaElcFile(const QString& path,
                                  QStringList &channelNames,
                                  QList<QVector<float> > &location3D,
                                  QList<QVector<float> > &location2D,
                                  QString &unit)
{
    //Open .elc file
    if(!path.contains(".elc"))
        return false;

    QFile file(path);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug()<<"Error opening elc file";
        return false;
    }

    //Start reading from file
    double numberElectrodes;
    QTextStream in(&file);
    bool read2D = false;

    while(!in.atEnd())
    {
        QString line = in.readLine();

        QStringList fields = line.split(QRegExp("\\s+"));

        //Delete last element if it is a blank character
        if(fields.at(fields.size()-1) == "")
            fields.removeLast();

        if(!line.contains("#")) //Skip commented areas in file
        {
            //Read number of electrodes
            if(line.contains("NumberPositions"))
                numberElectrodes = fields.at(1).toDouble();

            //Read the unit of the position values
            if(line.contains("UnitPosition"))
                unit = fields.at(1);

            //Read actual electrode positions
            if(line.contains("Positions2D"))
                read2D = true;

            if(line.contains(":") && !read2D) //Read 3D positions
            {
                channelNames.push_back(fields.at(0));
                QVector<float> posTemp;

                posTemp.push_back(fields.at(fields.size()-3).toDouble());    //x
                posTemp.push_back(fields.at(fields.size()-2).toDouble());    //y
                posTemp.push_back(fields.at(fields.size()-1).toDouble());    //z

                location3D.append(posTemp);
            }

            if(line.contains(":") && read2D) //Read 2D positions
            {
                QVector<float> posTemp;
                posTemp.push_back(fields.at(fields.size()-2).toDouble());    //x
                posTemp.push_back(fields.at(fields.size()-1).toDouble());    //y
                location2D.append(posTemp);
            }

            //Read channel names
            if(line.contains("Labels"))
            {
                line = in.readLine();
                fields = line.split(QRegExp("\\s+"));

                //Delete last element if it is a blank character
                if(fields.at(fields.size()-1) == "")
                    fields.removeLast();

                channelNames = fields;
            }
        }
    }

    Q_UNUSED(numberElectrodes);

    file.close();

    return true;
}


//*************************************************************************************************************

bool LayoutLoader::readMNELoutFile(const QString &path, QMap<QString, QPointF> &channelData)
{
    //Open .elc file
    if(!path.contains(".lout"))
        return false;

    channelData.clear();

    QFile file(path);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug()<<"Error opening mne lout file";
        return false;
    }

    //Start reading from file
    QTextStream in(&file);

    //skip first line
    in.readLine();

    while(!in.atEnd()) {
        QString line = in.readLine();

        QStringList fields = line.split(QRegExp("\\s+"));

        //Delete last element if it is a blank character
        if(fields.at(fields.size()-1) == "")
            fields.removeLast();

        QPointF posTemp;
        posTemp.setX(fields.at(1).toDouble());      //x
        posTemp.setY(fields.at(2).toDouble());      //y

        //Create channel data map entry
        QString key = QString("%1 %2").arg(fields.at(fields.size()-2)).arg(fields.at(fields.size()-1));
        channelData.insert(key, posTemp);
    }

    file.close();

    return true;
}
