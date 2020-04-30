//=============================================================================================================
/**
 * @file     surface.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     March, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the Surface class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "surface.h"
#include <utils/ioutils.h>

#include <iostream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QDataStream>
#include <QTextStream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace FSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Surface::Surface()
: m_sFilePath("")
, m_sFileName("")
, m_iHemi(-1)
, m_sSurf("")
, m_vecOffset(Vector3f::Zero(3))
{
}

//=============================================================================================================

Surface::Surface(const QString& p_sFile)
: m_sFilePath("")
, m_sFileName("")
, m_iHemi(-1)
, m_sSurf("")
, m_vecOffset(Vector3f::Zero(3))
{
    Surface::read(p_sFile, *this);
}

//=============================================================================================================

Surface::Surface(const QString &subject_id, qint32 hemi, const QString &surf, const QString &subjects_dir)
: m_sFilePath("")
, m_sFileName("")
, m_iHemi(-1)
, m_sSurf("")
, m_vecOffset(Vector3f::Zero(3))
{
    Surface::read(subject_id, hemi, surf, subjects_dir, *this);
}

//=============================================================================================================

Surface::Surface(const QString &path, qint32 hemi, const QString &surf)
: m_sFilePath("")
, m_sFileName("")
, m_iHemi(-1)
, m_sSurf("")
, m_vecOffset(Vector3f::Zero(3))
{
    Surface::read(path, hemi, surf, *this);
}

//=============================================================================================================

Surface::~Surface()
{
}

//=============================================================================================================

void Surface::clear()
{
    m_sFilePath.clear();
    m_sFileName.clear();
    m_iHemi = -1;
    m_sSurf.clear();
    m_matRR.resize(0,3);
    m_matTris.resize(0,3);
    m_matNN.resize(0,3);
    m_vecCurv.resize(0);
}

//=============================================================================================================

MatrixX3f Surface::compute_normals(const MatrixX3f& rr, const MatrixX3i& tris)
{
    printf("\tcomputing normals\n");
    // first, compute triangle normals
    MatrixX3f r1(tris.rows(),3); MatrixX3f r2(tris.rows(),3); MatrixX3f r3(tris.rows(),3);

    for(qint32 i = 0; i < tris.rows(); ++i)
    {
        r1.row(i) = rr.row(tris(i, 0));
        r2.row(i) = rr.row(tris(i, 1));
        r3.row(i) = rr.row(tris(i, 2));
    }

    MatrixX3f x = r2 - r1;
    MatrixX3f y = r3 - r1;
    MatrixX3f tri_nn(x.rows(),y.cols());
    tri_nn.col(0) = x.col(1).cwiseProduct(y.col(2)) - x.col(2).cwiseProduct(y.col(1));
    tri_nn.col(1) = x.col(2).cwiseProduct(y.col(0)) - x.col(0).cwiseProduct(y.col(2));
    tri_nn.col(2) = x.col(0).cwiseProduct(y.col(1)) - x.col(1).cwiseProduct(y.col(0));

    //   Triangle normals and areas
    MatrixX3f tmp = tri_nn.cwiseProduct(tri_nn);
    VectorXf normSize = tmp.rowwise().sum();
    normSize = normSize.cwiseSqrt();

    for(qint32 i = 0; i < normSize.size(); ++i)
        if(normSize(i) != 0)
            tri_nn.row(i) /= normSize(i);

    MatrixX3f nn = MatrixX3f::Zero(rr.rows(), 3);

    for(qint32 p = 0; p < tris.rows(); ++p)
    {
        Vector3i verts = tris.row(p);
        for(qint32 j = 0; j < verts.size(); ++j)
            nn.row(verts(j)) = tri_nn.row(p);
    }

    tmp = nn.cwiseProduct(nn);
    normSize = tmp.rowwise().sum();
    normSize = normSize.cwiseSqrt();

    for(qint32 i = 0; i < normSize.size(); ++i)
        if(normSize(i) != 0)
            nn.row(i) /= normSize(i);

    return nn;
}

//=============================================================================================================

bool Surface::read(const QString &subject_id, qint32 hemi, const QString &surf, const QString &subjects_dir, Surface &p_Surface, bool p_bLoadCurvature)
{
    if(hemi != 0 && hemi != 1)
        return false;

    QString p_sFile = QString("%1/%2/surf/%3.%4").arg(subjects_dir).arg(subject_id).arg(hemi == 0 ? "lh" : "rh").arg(surf);

    return read(p_sFile, p_Surface, p_bLoadCurvature);
}

//=============================================================================================================

bool Surface::read(const QString &path, qint32 hemi, const QString &surf, Surface &p_Surface, bool p_bLoadCurvature)
{
    if(hemi != 0 && hemi != 1)
        return false;

    QString p_sFile = QString("%1/%2.%3").arg(path).arg(hemi == 0 ? "lh" : "rh").arg(surf);

    return read(p_sFile, p_Surface, p_bLoadCurvature);
}

//=============================================================================================================

bool Surface::read(const QString &p_sFile, Surface &p_Surface, bool p_bLoadCurvature)
{
    p_Surface.clear();

    QFile t_File(p_sFile);

    if (!t_File.open(QIODevice::ReadOnly))
    {
        printf("\tError: Couldn't open the surface file\n");
        return false;
    }

    printf("Reading surface...\n");

    //Strip file name and path
    qint32 t_NameIdx = 0;
    if(p_sFile.contains("lh."))
        t_NameIdx = p_sFile.indexOf("lh.");
    else if(p_sFile.contains("rh."))
        t_NameIdx = p_sFile.indexOf("rh.");
    else
        return false;

    p_Surface.m_sFilePath = p_sFile.mid(0,t_NameIdx);
    p_Surface.m_sFileName = p_sFile.mid(t_NameIdx,p_sFile.size()-t_NameIdx);

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

    qint32 magic = IOUtils::fread3(t_DataStream);

    qint32 nvert = 0;
    qint32 nquad = 0;
    qint32 nface = 0;
    MatrixXf verts;
    MatrixXi faces;

    if(magic == QUAD_FILE_MAGIC_NUMBER || magic == NEW_QUAD_FILE_MAGIC_NUMBER)
    {
        nvert = IOUtils::fread3(t_DataStream);
        nquad = IOUtils::fread3(t_DataStream);
        if(magic == QUAD_FILE_MAGIC_NUMBER)
            printf("\t%s is a quad file (nvert = %d nquad = %d)\n", p_sFile.toUtf8().constData(),nvert,nquad);
        else
            printf("\t%s is a new quad file (nvert = %d nquad = %d)\n", p_sFile.toUtf8().constData(),nvert,nquad);

        //vertices
        verts.resize(nvert, 3);
        if(magic == QUAD_FILE_MAGIC_NUMBER)
        {
            qint16 iVal;
            for(qint32 i = 0; i < nvert; ++i)
            {
                for(qint32 j = 0; j < 3; ++j)
                {
                    t_DataStream >> iVal;
                    IOUtils::swap_short(iVal);
                    verts(i,j) = ((float)iVal) / 100;
                }
            }
        }
        else
        {
            t_DataStream.readRawData((char *)verts.data(), nvert*3*sizeof(float));
            for(qint32 i = 0; i < nvert; ++i)
                for(qint32 j = 0; j < 3; ++j)
                    IOUtils::swap_floatp(&verts(i,j));
        }

        MatrixXi quads = IOUtils::fread3_many(t_DataStream, nquad*4);
        MatrixXi quads_new(4, nquad);
        qint32 count = 0;
        for(qint32 j = 0; j < quads.cols(); ++j)
        {
            for(qint32 i = 0; i < quads.rows(); ++i)
            {
                quads_new(i,j) = quads(count, 0);
                ++count;
            }
        }
        quads = quads_new.transpose();
        //
        //  Face splitting follows
        //
        faces = MatrixXi::Zero(2*nquad,3);
        for(qint32 k = 0; k < nquad; ++k)
        {
            RowVectorXi quad = quads.row(k);
            if ((quad[0] % 2) == 0)
            {
                faces(nface,0) = quad[0];
                faces(nface,1) = quad[1];
                faces(nface,2) = quad[3];
                ++nface;

                faces(nface,0) = quad[2];
                faces(nface,1) = quad[3];
                faces(nface,2) = quad[1];
                ++nface;
            }
            else
            {
                faces(nface,0) = quad(0);
                faces(nface,1) = quad(1);
                faces(nface,2) = quad(2);
                ++nface;

                faces(nface,0) = quad(0);
                faces(nface,1) = quad(2);
                faces(nface,2) = quad(3);
                ++nface;
            }
        }
    }
    else if(magic == TRIANGLE_FILE_MAGIC_NUMBER)
    {
        QString s = t_File.readLine();
        t_File.readLine();

        t_DataStream >> nvert;
        t_DataStream >> nface;
        IOUtils::swap_int(nvert);
        IOUtils::swap_int(nface);

        printf("\t%s is a triangle file (nvert = %d ntri = %d)\n", p_sFile.toUtf8().constData(), nvert, nface);
        printf("\t%s", s.toUtf8().constData());

        //vertices
        verts.resize(3, nvert);
        t_DataStream.readRawData((char *)verts.data(), nvert*3*sizeof(float));
        for(qint32 i = 0; i < 3; ++i)
            for(qint32 j = 0; j < nvert; ++j)
                IOUtils::swap_floatp(&verts(i,j));

        //faces
        faces.resize(nface, 3);
        qint32 iVal;
        for(qint32 i = 0; i < nface; ++i)
        {
            for(qint32 j = 0; j < 3; ++j)
            {
                t_DataStream >> iVal;
                IOUtils::swap_int(iVal);
                faces(i,j) = iVal;
            }
        }
    }
    else
    {
        qWarning("Bad magic number (%d) in surface file %s",magic,p_sFile.toUtf8().constData());
        return false;
    }

    verts.transposeInPlace();
    verts.array() *= 0.001f;

    p_Surface.m_matRR = verts.block(0,0,verts.rows(),3);
    p_Surface.m_matTris = faces.block(0,0,faces.rows(),3);

    //-> not needed since qglbuilder is doing that for us
    p_Surface.m_matNN = compute_normals(p_Surface.m_matRR, p_Surface.m_matTris);

    // hemi info
    if(t_File.fileName().contains("lh."))
        p_Surface.m_iHemi = 0;
    else if(t_File.fileName().contains("rh."))
        p_Surface.m_iHemi = 1;
    else
    {
        p_Surface.m_iHemi = -1;
        return false;
    }

    //Loaded surface
    p_Surface.m_sSurf = t_File.fileName().mid((t_NameIdx+3),t_File.fileName().size() - (t_NameIdx+3));

    //Load curvature
    if(p_bLoadCurvature)
    {
        QString t_sCurvatureFile = QString("%1%2.curv").arg(p_Surface.m_sFilePath).arg(p_Surface.m_iHemi == 0 ? "lh" : "rh");
        printf("\t");
        p_Surface.m_vecCurv = Surface::read_curv(t_sCurvatureFile);
    }

    t_File.close();
    printf("\tRead a surface with %d vertices from %s\n[done]\n",nvert,p_sFile.toUtf8().constData());

    return true;
}

//=============================================================================================================

VectorXf Surface::read_curv(const QString &p_sFileName)
{
    VectorXf curv;

    printf("Reading curvature...");
    QFile t_File(p_sFileName);

    if (!t_File.open(QIODevice::ReadOnly))
    {
        printf("\tError: Couldn't open the curvature file\n");
        return curv;
    }

    QDataStream t_DataStream(&t_File);
    t_DataStream.setByteOrder(QDataStream::BigEndian);

    qint32 vnum = IOUtils::fread3(t_DataStream);
    qint32 NEW_VERSION_MAGIC_NUMBER = 16777215;

    if(vnum == NEW_VERSION_MAGIC_NUMBER)
    {
        qint32 fnum, vals_per_vertex;
        t_DataStream >> vnum;

        t_DataStream >> fnum;
        t_DataStream >> vals_per_vertex;

        curv.resize(vnum, 1);
        t_DataStream.readRawData((char *)curv.data(), vnum*sizeof(float));
        for(qint32 i = 0; i < vnum; ++i)
           IOUtils::swap_floatp(&curv(i));
    }
    else
    {
        qint32 fnum = IOUtils::fread3(t_DataStream);
        Q_UNUSED(fnum)
        qint16 iVal;
        curv.resize(vnum, 1);
        for(qint32 i = 0; i < vnum; ++i)
        {
            t_DataStream >> iVal;
            IOUtils::swap_short(iVal);
            curv(i) = ((float)iVal) / 100;
        }
    }
    t_File.close();

    printf("[done]\n");

    return curv;
}
