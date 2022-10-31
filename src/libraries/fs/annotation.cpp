//=============================================================================================================
/**
 * @file     annotation.cpp
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
 * @brief    Definition of the Annotation class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "annotation.h"
#include "label.h"
#include "surface.h"

#include <iostream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QFile>
#include <QDataStream>
#include <QFileInfo>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Annotation::Annotation()
: m_sFilePath("")
, m_sFileName("")
, m_iHemi(-1)
{
}

//=============================================================================================================

Annotation::Annotation(const QString& p_sFileName)
: m_sFileName(p_sFileName)
{
    Annotation t_Annotation;
    Annotation::read(m_sFileName, t_Annotation);
     *this = t_Annotation;
}

//=============================================================================================================

Annotation::Annotation(const QString &subject_id, qint32 hemi, const QString &atlas, const QString &subjects_dir)
: m_sFilePath("")
, m_sFileName("")
, m_iHemi(-1)
{
    Annotation::read(subject_id, hemi, atlas, subjects_dir, *this);
}

//=============================================================================================================

Annotation::Annotation(const QString &path, qint32 hemi, const QString &atlas)
: m_sFilePath("")
, m_sFileName("")
, m_iHemi(-1)
{
    Annotation::read(path, hemi, atlas, *this);
}

//=============================================================================================================

Annotation::~Annotation()
{
}

//=============================================================================================================

void Annotation::clear()
{
    m_sFileName = QString("");
    m_Vertices = VectorXi::Zero(0);
    m_LabelIds = VectorXi::Zero(0);
    m_Colortable.clear();
}

//=============================================================================================================

bool Annotation::read(const QString &subject_id, qint32 hemi, const QString &atlas, const QString &subjects_dir, Annotation &p_Annotation)
{
    if(hemi != 0 && hemi != 1)
        return false;

    QString p_sFile = QString("%1/%2/label/%3.%4.annot").arg(subjects_dir).arg(subject_id).arg(hemi == 0 ? "lh" : "rh").arg(atlas);

    return read(p_sFile, p_Annotation);
}

//=============================================================================================================

bool Annotation::read(const QString &path, qint32 hemi, const QString &atlas, Annotation &p_Annotation)
{
    if(hemi != 0 && hemi != 1)
        return false;

    QString p_sFile = QString("%1/%2.%3.annot").arg(path).arg(hemi == 0 ? "lh" : "rh").arg(atlas);

    return read(p_sFile, p_Annotation);
}

//=============================================================================================================

bool Annotation::read(const QString& p_sFileName, Annotation &p_Annotation)
{
    p_Annotation.clear();

    printf("Reading annotation...\n");
    QFile t_File(p_sFileName);
    QFileInfo fileInfo(t_File.fileName());

    p_Annotation.m_sFileName = fileInfo.fileName();
    p_Annotation.m_sFilePath = fileInfo.filePath();

    if (!t_File.open(QIODevice::ReadOnly))
    {
        printf("\tError: Couldn't open the file\n");
        return false;
    }

    QDataStream t_Stream(&t_File);
    t_Stream.setByteOrder(QDataStream::BigEndian);

    qint32 numEl;
    t_Stream >> numEl;

    p_Annotation.m_Vertices = VectorXi(numEl);
    p_Annotation.m_LabelIds = VectorXi(numEl);

    for(qint32 i = 0; i < numEl; ++i)
    {
        t_Stream >> p_Annotation.m_Vertices[i];
        t_Stream >> p_Annotation.m_LabelIds[i];
    }

    qint32 hasColortable;
    t_Stream >> hasColortable;
    if (hasColortable)
    {
        p_Annotation.m_Colortable.clear();

        //Read colortable
        qint32 numEntries;
        t_Stream >> numEntries;
        qint32 len;
        if(numEntries > 0)
        {

            printf("\tReading from Original Version\n");
            p_Annotation.m_Colortable.numEntries = numEntries;
            t_Stream >> len;
            QByteArray tmp;
            tmp.resize(len);
            t_Stream.readRawData(tmp.data(),len);
            p_Annotation.m_Colortable.orig_tab = tmp;

            for(qint32 i = 0; i < numEntries; ++i)
                p_Annotation.m_Colortable.struct_names.append("");

            p_Annotation.m_Colortable.table = MatrixXi(numEntries,5);

            for(qint32 i = 0; i < numEntries; ++i)
            {
                t_Stream >> len;
                tmp.resize(len);
                t_Stream.readRawData(tmp.data(),len);

                p_Annotation.m_Colortable.struct_names[i]= tmp;

                for(qint32 j = 0; j < 4; ++j)
                    t_Stream >> p_Annotation.m_Colortable.table(i,j);

                p_Annotation.m_Colortable.table(i,4) = p_Annotation.m_Colortable.table(i,0)
                        + p_Annotation.m_Colortable.table(i,1) * 256       //(2^8)
                        + p_Annotation.m_Colortable.table(i,2) * 65536     //(2^16)
                        + p_Annotation.m_Colortable.table(i,3) * 16777216; //(2^24);
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
            p_Annotation.m_Colortable.numEntries = numEntries;

            t_Stream >> len;
            QByteArray tmp;
            tmp.resize(len);
            t_Stream.readRawData(tmp.data(),len);
            p_Annotation.m_Colortable.orig_tab = tmp;

            for(qint32 i = 0; i < numEntries; ++i)
                p_Annotation.m_Colortable.struct_names.append("");

            p_Annotation.m_Colortable.table = MatrixXi(numEntries,5);

            qint32 numEntriesToRead;
            t_Stream >> numEntriesToRead;

            qint32 structure;
            for(qint32 i = 0; i < numEntriesToRead; ++i)
            {

                t_Stream >> structure;
                if (structure < 0)
                    printf("\tError! Read entry, index %d\n", structure);

                if(!p_Annotation.m_Colortable.struct_names[structure].isEmpty())
                    printf("Error! Duplicate Structure %d", structure);

                t_Stream >> len;
                tmp.resize(len);
                t_Stream.readRawData(tmp.data(),len);

                p_Annotation.m_Colortable.struct_names[structure]= tmp;

                for(qint32 j = 0; j < 4; ++j)
                    t_Stream >> p_Annotation.m_Colortable.table(structure,j);

                p_Annotation.m_Colortable.table(structure,4) = p_Annotation.m_Colortable.table(structure,0)
                        + p_Annotation.m_Colortable.table(structure,1) * 256       //(2^8)
                        + p_Annotation.m_Colortable.table(structure,2) * 65536     //(2^16)
                        + p_Annotation.m_Colortable.table(structure,3) * 16777216; //(2^24);
            }
        }
        printf("\tcolortable with %d entries read\n\t(originally %s)\n", p_Annotation.m_Colortable.numEntries, p_Annotation.m_Colortable.orig_tab.toUtf8().constData());
    }
    else
    {
        printf("\tError! No colortable stored\n");
    }

    // hemi info
    if(t_File.fileName().contains("lh."))
        p_Annotation.m_iHemi = 0;
    else
        p_Annotation.m_iHemi = 1;

    printf("[done]\n");

    t_File.close();

    return true;
}

//=============================================================================================================

bool Annotation::toLabels(const Surface &p_surf,
                          QList<Label> &p_qListLabels,
                          QList<RowVector4i> &p_qListLabelRGBAs,
                          const QStringList& lLabelPicks) const
{
    if(this->m_iHemi != p_surf.hemi())
    {
        qWarning("Annotation and surface hemisphere (annot = %d; surf = %d) do not match!\n", this->m_iHemi, p_surf.hemi());
        return false;
    }

    if(m_LabelIds.size() == 0)
    {
        qWarning("Annotation doesn't' contain data!\n");
        return false;
    }

    printf("Converting labels from annotation...");

//n_read = 0
//labels = list()
//label_colors = list()

    VectorXi label_ids = m_Colortable.getLabelIds();
    QStringList label_names = m_Colortable.getNames();
    MatrixX4i label_rgbas = m_Colortable.getRGBAs();

    // load the vertex positions from surface
    MatrixX3f vert_pos = p_surf.rr();

//    qDebug() << label_rgbas.rows() << label_ids.size() << label_names.size();

//    std::cout << label_ids;

    qint32 label_id, count;
    RowVector4i label_rgba;
    VectorXi vertices;
    VectorXd values;
    MatrixX3f pos;
    QString name;
    for(qint32 i = 0; i < label_rgbas.rows(); ++i)
    {
        label_id = label_ids[i];
        label_rgba = label_rgbas.row(i);
        count = 0;
        vertices.resize(m_LabelIds.size());
        //Where
        for(qint32 j = 0; j < m_LabelIds.size(); ++j)
        {
            if(m_LabelIds[j] == label_id)
            {
                vertices[count] = j;
                ++count;
            }
        }
        // check if label is part of cortical surface
        if(count == 0)
            continue;
        vertices.conservativeResize(count);

        pos.resize(count, 3);
        for(qint32 j = 0; j < count; ++j)
            pos.row(j) = vert_pos.row(vertices[j]);

        values = VectorXd::Zero(count);
        name = QString("%1-%2").arg(label_names[i]).arg(this->m_iHemi == 0 ? "lh" : "rh");

        // put it all together
        if(lLabelPicks.isEmpty()) {
            //t_tris
            p_qListLabels.append(Label(vertices, pos, values, this->m_iHemi, name, label_id));
            // store the color
            p_qListLabelRGBAs.append(label_rgba);
        } else if (lLabelPicks.indexOf(name) != -1) {
            //t_tris
            p_qListLabels.append(Label(vertices, pos, values, this->m_iHemi, name, label_id));
            // store the color
            p_qListLabelRGBAs.append(label_rgba);
        }
    }

//    for label_id, label_name, label_rgba in
//            zip(label_ids, label_names, label_rgbas):
//        vertices = np.where(annot == label_id)[0]
//        if len(vertices) == 0:
//            # label is not part of cortical surface
//            continue
//        pos = vert_pos[vertices, :]
//        values = np.zeros(len(vertices))
//        name = label_name + '-' + hemi
//        label = Label(vertices, pos, values, hemi, name=name)
//        labels.append(label)

//        # store the color
//        label_rgba = tuple(label_rgba / 255.)
//        label_colors.append(label_rgba)

//    n_read = len(labels) - n_read
//    logger.info('   read %d labels from %s' % (n_read, fname))

//# sort the labels and colors by label name
//names = [label.name for label in labels]
//labels, label_colors = zip(*((label, color) for (name, label, color)
//                           in sorted(zip(names, labels, label_colors))))
//# convert tuples to lists
//labels = list(labels)
//label_colors = list(label_colors)

    printf("[done]\n");

    return true;
}
