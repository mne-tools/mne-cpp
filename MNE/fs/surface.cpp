//=============================================================================================================
/**
* @file     surface.cpp
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
* @brief    Implementation of the Surface class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "surface.h"
#include <utils/mnemath.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QFile>
#include <QDataStream>
#include <QTextStream>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace FSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Surface::Surface()
{
}


//*************************************************************************************************************

Surface::~Surface()
{
}


//*************************************************************************************************************

void Surface::clear()
{
    verts = Matrix3Xf();
    faces = Matrix3Xf();
}


//*************************************************************************************************************

bool Surface::read(const QString &p_sFileName, Surface &p_Surface)
{
    p_Surface.clear();

    printf("Reading surface...\n");
    QFile t_File(p_sFileName);

    if (!t_File.open(QIODevice::ReadOnly))
    {
        printf("\tError: Couldn't open the file\n");
        return false;
    }

    QDataStream t_DataStream(&t_File);
    t_DataStream.setByteOrder(QDataStream::BigEndian);

    //
    //   Magic numbers to identify QUAD and TRIANGLE files
    //
    //   QUAD_FILE_MAGIC_NUMBER =  (-1 & 0x00ffffff) ;
    //   NEW_QUAD_FILE_MAGIC_NUMBER =  (-3 & 0x00ffffff) ;
    //
    qint32 NEW_QUAD_FILE_MAGIC_NUMBER =  16777213;
    qint32 TRIANGLE_FILE_MAGIC_NUMBER =  16777214;
    qint32 QUAD_FILE_MAGIC_NUMBER     =  16777215;

    qint32 magic = MNEMath::fread3(t_DataStream);

    if(magic == QUAD_FILE_MAGIC_NUMBER || magic == NEW_QUAD_FILE_MAGIC_NUMBER)
    {
        qint32 nvert = MNEMath::fread3(t_DataStream);
        qint32 nquad = MNEMath::fread3(t_DataStream);
//        if magic == QUAD_FILE_MAGIC_NUMBER
//            fprintf(1,'\t%s is a quad file (nvert = %d nquad = %d)\n', ...
//                fname,nvert,nquad);
//        else
//            fprintf(1,'\t%s is a new quad file (nvert = %d nquad = %d)\n', ...
//                fname,nvert,nquad);
//        end
//        if magic == QUAD_FILE_MAGIC_NUMBER
//            verts = fread(fid, nvert*3, 'int16') ./ 100 ;
//        else
//            verts = fread(fid, nvert*3, 'single');
//        end
//        if (nargout > 1)
//            quads = fread3_many(fid,nquad*4);
//            quads = reshape(quads,4,nquad)';
//            %
//            %   Face splitting follows
//            %
//            faces = zeros(2*nquad,3);
//            nface = 0;
//            for k = 1:nquad
//                quad = quads(k,:);
//                if mod(quad(1), 2) == 0
//                    nface = nface + 1;
//                    faces(nface,1) = quad(1);
//                    faces(nface,2) = quad(2);
//                    faces(nface,3) = quad(4);

//                    nface = nface + 1;
//                    faces(nface,1) = quad(3);
//                    faces(nface,2) = quad(4);
//                    faces(nface,3) = quad(2);
//                else
//                    nface = nface + 1;
//                    faces(nface,1) = quad(1);
//                    faces(nface,2) = quad(2);
//                    faces(nface,3) = quad(3);

//                    nface = nface + 1;
//                    faces(nface,1) = quad(1);
//                    faces(nface,2) = quad(3);
//                    faces(nface,3) = quad(4);
//                end
//            end
//            faces = faces + 1;                   % Our numbering starts from one
//        end
    }
    else if(magic == TRIANGLE_FILE_MAGIC_NUMBER)
    {
        QTextStream t_TextStream(&t_File);
        QString s = t_TextStream.readLine();
        t_TextStream.readLine();

        qint32 nvert, nface;
        t_DataStream >> nvert;
        t_DataStream >> nface;

        qDebug() << nvert << nface;


    }








//    p_Annotation.m_Vertices = VectorXi(numEl);
//    p_Annotation.m_Label = VectorXi(numEl);

//    for(qint32 i = 0; i < numEl; ++i)
//    {
//        t_Stream >> p_Annotation.m_Vertices[i];
//        t_Stream >> p_Annotation.m_Label[i];
//    }

////    std::cout << "Vertices" << std::endl << m_pVertices->block(0,0,10,1) << std::endl;
////    std::cout << "Label" << std::endl << m_pLabel->block(0,0,10,1) << std::endl;

//    qint32 hasColortable;
//    t_Stream >> hasColortable;
//    if (hasColortable)
//    {
//        p_Annotation.m_Colortable.clear();

//        //Read colortable
//        qint32 numEntries;
//        t_Stream >> numEntries;
//        qint32 len;
//        if(numEntries > 0)
//        {

//            printf("\tReading from Original Version\n");
//            p_Annotation.m_Colortable.numEntries = numEntries;
//            t_Stream >> len;
//            QByteArray tmp;
//            tmp.resize(len);
//            t_Stream.readRawData(tmp.data(),len);
//            p_Annotation.m_Colortable.orig_tab = tmp;

//            for(qint32 i = 0; i < numEntries; ++i)
//                p_Annotation.m_Colortable.struct_names.append("");

//            p_Annotation.m_Colortable.table = MatrixXi(numEntries,5);

//            for(qint32 i = 0; i < numEntries; ++i)
//            {
//                t_Stream >> len;
//                tmp.resize(len);
//                t_Stream.readRawData(tmp.data(),len);

//                p_Annotation.m_Colortable.struct_names[i]= tmp;

//                for(qint32 j = 0; j < 4; ++j)
//                    t_Stream >> p_Annotation.m_Colortable.table(i,j);

//                p_Annotation.m_Colortable.table(i,4) = p_Annotation.m_Colortable.table(i,0)
//                        + p_Annotation.m_Colortable.table(i,1) * 256       //(2^8)
//                        + p_Annotation.m_Colortable.table(i,2) * 65536     //(2^16)
//                        + p_Annotation.m_Colortable.table(i,3) * 16777216; //(2^24);
//            }
//        }
//        else
//        {
//            qint32 version = -numEntries;
//            if(version != 2)
//                printf("\tError! Does not handle version %d\n", version);
//            else
//                printf("\tReading from version %d\n", version);

//            t_Stream >> numEntries;
//            p_Annotation.m_Colortable.numEntries = numEntries;

//            t_Stream >> len;
//            QByteArray tmp;
//            tmp.resize(len);
//            t_Stream.readRawData(tmp.data(),len);
//            p_Annotation.m_Colortable.orig_tab = tmp;

//            for(qint32 i = 0; i < numEntries; ++i)
//                p_Annotation.m_Colortable.struct_names.append("");

//            p_Annotation.m_Colortable.table = MatrixXi(numEntries,5);

//            qint32 numEntriesToRead;
//            t_Stream >> numEntriesToRead;

//            qint32 structure;
//            for(qint32 i = 0; i < numEntriesToRead; ++i)
//            {

//                t_Stream >> structure;
//                if (structure < 0)
//                    printf("\tError! Read entry, index %d\n", structure);

//                if(!p_Annotation.m_Colortable.struct_names[structure].isEmpty())
//                    printf("Error! Duplicate Structure %d", structure);

//                t_Stream >> len;
//                tmp.resize(len);
//                t_Stream.readRawData(tmp.data(),len);

//                p_Annotation.m_Colortable.struct_names[structure]= tmp;

//                for(qint32 j = 0; j < 4; ++j)
//                    t_Stream >> p_Annotation.m_Colortable.table(structure,j);

//                p_Annotation.m_Colortable.table(structure,4) = p_Annotation.m_Colortable.table(structure,0)
//                        + p_Annotation.m_Colortable.table(structure,1) * 256       //(2^8)
//                        + p_Annotation.m_Colortable.table(structure,2) * 65536     //(2^16)
//                        + p_Annotation.m_Colortable.table(structure,3) * 16777216; //(2^24);
//            }
//        }
//        printf("\tcolortable with %d entries read\n\t(originally %s)\n", p_Annotation.m_Colortable.numEntries, p_Annotation.m_Colortable.orig_tab.toUtf8().constData());
//    }
//    else
//    {
//        printf("\tError! No colortable stored\n");
//    }

    printf("[done]\n");

    t_File.close();

    return true;
}

