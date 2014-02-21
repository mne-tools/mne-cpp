//=============================================================================================================
/**
* @file     asaelc.cpp
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     November, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Implementation of the AsAElc class
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "asaelc.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AsAElc::AsAElc()
{
}


//*************************************************************************************************************

bool AsAElc::readElcFile(QString path, QStringList &channelNames, QVector<QVector<double> > &location3D, QVector<QVector<double> > &location2D, QString &unit)
{
    //Open .elc file
    if(!path.contains(".elc"))
        return false;

    QFile file(path);
    file.open(QIODevice::ReadOnly | QIODevice::Text);

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
                QVector<double> posTemp;

                posTemp.push_back(fields.at(fields.size()-3).toDouble());    //x
                posTemp.push_back(fields.at(fields.size()-2).toDouble());    //y
                posTemp.push_back(fields.at(fields.size()-1).toDouble());    //z

                location3D.push_back(posTemp);
            }

            if(line.contains(":") && read2D) //Read 2D positions
            {
                QVector<double> posTemp;
                posTemp.push_back(fields.at(fields.size()-2).toDouble());    //x
                posTemp.push_back(fields.at(fields.size()-1).toDouble());    //y
                location2D.push_back(posTemp);
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
