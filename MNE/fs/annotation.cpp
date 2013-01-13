//=============================================================================================================
/**
* @file     annotation.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
*           Bruce Fischl
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    ToDo Documentation...
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "annotation.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QFile>
#include <QDataStream>

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

Annotation::Annotation(QString& p_sFileName)
: m_sFileName(p_sFileName)
{
    read_annotation(m_sFileName);
}


//*************************************************************************************************************

Annotation::~Annotation()
{

}


//*************************************************************************************************************

void Annotation::read_annotation(QString& t_sFileName)
{
    printf("Reading annotation...\n");
    QFile t_File(t_sFileName);

    if (!t_File.open(QIODevice::ReadOnly))
    {
        printf("\tError: Couldn't open the file\n");
        return;
    }

    QDataStream t_Stream(&t_File);
    t_Stream.setByteOrder(QDataStream::BigEndian);

    qint32 numEl;
    t_Stream >> numEl;

    m_Vertices = VectorXi(numEl);
    m_Label = VectorXi(numEl);

    for(qint32 i = 0; i < numEl; ++i)
    {
        t_Stream >> m_Vertices[i];
        t_Stream >> m_Label[i];
    }

//    std::cout << "Vertices" << std::endl << m_pVertices->block(0,0,10,1) << std::endl;
//    std::cout << "Label" << std::endl << m_pLabel->block(0,0,10,1) << std::endl;

    qint32 hasColortable;
    t_Stream >> hasColortable;
    if (hasColortable)
    {
        m_Colortable.clear();

        //Read colortable
        qint32 numEntries;
        t_Stream >> numEntries;
        qint32 len;
        if(numEntries > 0)
        {

            printf("\tReading from Original Version\n");
            m_Colortable.numEntries = numEntries;
            t_Stream >> len;
            QByteArray tmp;
            tmp.resize(len);
            t_Stream.readRawData(tmp.data(),len);
            m_Colortable.orig_tab = tmp;

            for(qint32 i = 0; i < numEntries; ++i)
                m_Colortable.struct_names.append("");

            m_Colortable.table = MatrixXi(numEntries,5);

            for(qint32 i = 0; i < numEntries; ++i)
            {
                t_Stream >> len;
                tmp.resize(len);
                t_Stream.readRawData(tmp.data(),len);

                m_Colortable.struct_names[i]= tmp;

                for(qint32 j = 0; j < 4; ++j)
                    t_Stream >> m_Colortable.table(i,j);

                m_Colortable.table(i,4) = m_Colortable.table(i,0)
                        + m_Colortable.table(i,1) * 256       //(2^8)
                        + m_Colortable.table(i,2) * 65536     //(2^16)
                        + m_Colortable.table(i,3) * 16777216; //(2^24);
            }
        }
        else
        {
            qint32 version = -numEntries;
            if(version != 2)
                printf("\tError! Does not handle version %d\n", version);
            else
                printf("\tReading from version %d\n", version);

            t_Stream >> numEntries;
            m_Colortable.numEntries = numEntries;

            t_Stream >> len;
            QByteArray tmp;
            tmp.resize(len);
            t_Stream.readRawData(tmp.data(),len);
            m_Colortable.orig_tab = tmp;

            for(qint32 i = 0; i < numEntries; ++i)
                m_Colortable.struct_names.append("");

            m_Colortable.table = MatrixXi(numEntries,5);

            qint32 numEntriesToRead;
            t_Stream >> numEntriesToRead;

            qint32 structure;
            for(qint32 i = 0; i < numEntriesToRead; ++i)
            {

                t_Stream >> structure;
                if (structure < 0)
                    printf("\tError! Read entry, index %d\n", structure);

                if(!m_Colortable.struct_names[structure].isEmpty())
                    printf("Error! Duplicate Structure %d", structure);

                t_Stream >> len;
                tmp.resize(len);
                t_Stream.readRawData(tmp.data(),len);

                m_Colortable.struct_names[structure]= tmp;

                for(qint32 j = 0; j < 4; ++j)
                    t_Stream >> m_Colortable.table(structure,j);

                m_Colortable.table(structure,4) = m_Colortable.table(structure,0)
                        + m_Colortable.table(structure,1) * 256       //(2^8)
                        + m_Colortable.table(structure,2) * 65536     //(2^16)
                        + m_Colortable.table(structure,3) * 16777216; //(2^24);
            }
        }
        printf("\tcolortable with %d entries read\n\t(originally %s)\n", m_Colortable.numEntries, m_Colortable.orig_tab.toUtf8().constData());
    }
    else
    {
        printf("\tError! No colortable stored\n");
    }

    printf("[done]\n");

    t_File.close();
}

