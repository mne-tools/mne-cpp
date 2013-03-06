//=============================================================================================================
/**
* @file     label.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     March, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Implementation of the Label class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "label.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QDebug>

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Label::Label()
: hemi(-1)
{
}


//*************************************************************************************************************

Label::Label(const VectorXi &p_vertices, const MatrixX3f &p_pos, const VectorXd &p_values, qint32 p_hemi, const QString &p_name)
: vertices(p_vertices)
, pos(p_pos)
, values(p_values)
, hemi(p_hemi)
, name(p_name)
{

}


//*************************************************************************************************************

Label::~Label()
{
}


//*************************************************************************************************************

void Label::clear()
{
    comment = QString("");
    hemi = -1;
    name = QString("");
    vertices = VectorXi();
    pos = MatrixX3f();
    values = VectorXd();
//    vertices.clear();
//    pos.clear();
//    values.clear();
}


//*************************************************************************************************************

bool Label::read(const QString& p_sFileName, Label &p_Label)
{
    p_Label.clear();

    if(p_sFileName.mid(p_sFileName.size()-6,6).compare(".label") != 0)
    {
        qWarning("Given file (%s) is not a .label file!\n", p_sFileName.toLatin1().constData());
        return false;
    }

    printf("Reading label...");
    QFile t_File(p_sFileName);

    if (!t_File.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning("\tError: Couldn't open the label file\n");
        return false;
    }

    QTextStream t_TextStream(&t_File);

    QString comment = t_TextStream.readLine();
    qint32 nv = t_TextStream.readLine().toInt();

    MatrixXd data(nv, 5);

    QStringList list;
    qint32 count;
    bool isNumber;
    double value;
    for(qint32 i = 0; i < nv; ++i)
    {
        count = 0;
        list = t_TextStream.readLine().split(QRegExp("\\s+"), QString::SkipEmptyParts);

        for(qint32 j = 0; j < list.size(); ++j)
        {
            value = list[j].toDouble(&isNumber);
            if(isNumber)
            {
                data(i, count) = value;
                ++count;
            }
        }
    }

    p_Label.comment = comment.mid(1,comment.size()-1);
    if(t_File.fileName().contains("lh."))
        p_Label.hemi = 0;
    else
        p_Label.hemi = 1;

    //This structure is not need since we don't mix both hemis
//    p_Label.vertices.insert(p_Label.hemi, data.cast<int>().block(0,0,data.rows(),1));
//    p_Label.pos.insert(p_Label.hemi, data.cast<float>().block(0,1,data.rows(),3).array() * 1e-3);
//    p_Label.values.insert(p_Label.hemi, data.block(0,4,data.rows(),1));
    p_Label.vertices = data.cast<int>().block(0,0,data.rows(),1);
    p_Label.pos = data.cast<float>().block(0,1,data.rows(),3).array() * 1e-3;
    p_Label.values = data.block(0,4,data.rows(),1);

    if(t_File.fileName().contains("lh.label"))
    {
        QStringList tmpList = t_File.fileName().split("lh.")[0].split(QRegExp("\\W+"));
        p_Label.name = tmpList[tmpList.size()-1];
    }
    else if(t_File.fileName().contains("lh."))
        p_Label.name = t_File.fileName().split("lh.")[1].split(QRegExp("\\W+"))[0];

    printf("[done]\n");

    t_File.close();

    return true;
}
