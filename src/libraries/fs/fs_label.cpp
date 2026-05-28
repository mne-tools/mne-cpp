//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file fs_label.cpp
 * @since March 2026
 * @brief Implementation of @ref FSLIB::FsLabel: parses FreeSurfer/MNE ASCII .label files into vertex / position / value arrays.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_label.h"
#include "fs_surface.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QSet>
#include <QRegularExpression>
//#include <QDebug>

#include <iostream>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FsLabel::FsLabel()
: hemi(-1)
, label_id(-1)
{
}

//=============================================================================================================

FsLabel::FsLabel(const VectorXi &p_vertices,
             const MatrixX3f &p_pos,
             const VectorXd &p_values,
             qint32 p_hemi,
             const QString &p_name,
             qint32 p_id)
: vertices(p_vertices)
, pos(p_pos)
, values(p_values)
, hemi(p_hemi)
, name(p_name)
, label_id(p_id)
{
}

//=============================================================================================================

FsLabel::~FsLabel()
{
}

//=============================================================================================================

void FsLabel::clear()
{
    comment = QString("");
    hemi = -1;
    name = QString("");
    vertices = VectorXi();
    pos = MatrixX3f(0,3);
    values = VectorXd();

    label_id = -1;
}

//=============================================================================================================

MatrixX3i FsLabel::selectTris(const FsSurface & p_Surface)
{
//    //check whether there are data to create the tris
//    if(this->vertices.size() == 0)
//        return MatrixX3i(0,3);

//    MatrixX3i tris(p_Surface.tris().rows(),3);

//    QSet<int> verts;
//    verts.reserve(this->vertices.size());
//    for(qint32 i = 0; i < this->vertices.size(); ++i)
//        verts.insert(this->vertices[i]);

//    //
//    // Search for all the tris where is at least one corner part of the label
//    //
//    qint32 t_size = 0;
//    for(qint32 i = 0; i < p_Surface.tris().rows(); ++i)
//    {
//        if(verts.contains(p_Surface.tris()(i,0)) || verts.contains(p_Surface.tris()(i,1)) || verts.contains(p_Surface.tris()(i,2)))
//        {
//            tris.row(t_size) = p_Surface.tris().row(i);
//            ++t_size;
//        }
//    }

//    tris.conservativeResize(t_size, 3);

    return this->selectTris(p_Surface.tris());//tris;
}

//=============================================================================================================

MatrixX3i FsLabel::selectTris(const MatrixX3i &p_matTris)
{
    //check whether there are data to create the tris
    if(this->vertices.size() == 0)
        return MatrixX3i(0,3);

    MatrixX3i tris(p_matTris.rows(),3);

    QSet<int> verts;
    verts.reserve(this->vertices.size());
    for(qint32 i = 0; i < this->vertices.size(); ++i)
        verts.insert(this->vertices[i]);

    //
    // Search for all the tris where is at least one corner part of the label
    //
    qint32 t_size = 0;
    for(qint32 i = 0; i < p_matTris.rows(); ++i)
    {
        if(verts.contains(p_matTris(i,0)) || verts.contains(p_matTris(i,1)) || verts.contains(p_matTris(i,2)))
        {
            tris.row(t_size) = p_matTris.row(i);
            ++t_size;
        }
    }

    tris.conservativeResize(t_size, 3);

    return tris;
}

//=============================================================================================================

bool FsLabel::read(const QString& p_sFileName, FsLabel &p_Label)
{
    p_Label.clear();

    if(p_sFileName.mid(p_sFileName.size()-6,6).compare(".label") != 0)
    {
        qWarning("Given file (%s) is not a .label file!\n", p_sFileName.toUtf8().constData());
        return false;
    }

    qInfo("Reading label...");
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

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        auto skip = QString::SkipEmptyParts;
#else
        auto skip = Qt::SkipEmptyParts;
#endif

        list = t_TextStream.readLine().split(QRegularExpression("\\s+"), skip);

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
    p_Label.pos = data.cast<float>().block(0,1,data.rows(),3).array() * 1e-3f;
    p_Label.values = data.block(0,4,data.rows(),1);

    if(t_File.fileName().contains("lh.label"))
    {
        QStringList tmpList = t_File.fileName().split("lh.")[0].split(QRegularExpression("\\W+"));
        p_Label.name = tmpList[tmpList.size()-1];
    }
    else if(t_File.fileName().contains("lh."))
        p_Label.name = t_File.fileName().split("lh.")[1].split(QRegularExpression("\\W+"))[0];

    qInfo("[done]\n");

    t_File.close();

    return true;
}
