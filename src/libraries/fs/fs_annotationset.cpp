//=============================================================================================================
/**
 * @file     fs_annotationset.cpp
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
 * @brief     FsAnnotationSet class implementation
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_annotationset.h"
#include "fs_surfaceset.h"

#include <QFile>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FsAnnotationSet::FsAnnotationSet()
{
}

//=============================================================================================================

FsAnnotationSet::FsAnnotationSet(const QString &subject_id, qint32 hemi, const QString &atlas, const QString &subjects_dir)
{
    FsAnnotation t_Annotation;
    if(hemi == 0 || hemi == 1)
    {
        if(FsAnnotation::read(subject_id, hemi, atlas, subjects_dir, t_Annotation))
            insert(t_Annotation);
    }
    else if(hemi == 2)
    {
        if(FsAnnotation::read(subject_id, 0, atlas, subjects_dir, t_Annotation))
            insert(t_Annotation);
        if(FsAnnotation::read(subject_id, 1, atlas, subjects_dir, t_Annotation))
            insert(t_Annotation);
    }
}

//=============================================================================================================

FsAnnotationSet::FsAnnotationSet(const QString &path, qint32 hemi, const QString &atlas)
{
    FsAnnotation t_Annotation;
    if(hemi == 0 || hemi == 1)
    {
        if(FsAnnotation::read(path, hemi, atlas, t_Annotation))
            insert(t_Annotation);
    }
    else if(hemi == 2)
    {
        if(FsAnnotation::read(path, 0, atlas, t_Annotation))
            insert(t_Annotation);
        if(FsAnnotation::read(path, 1, atlas, t_Annotation))
            insert(t_Annotation);
    }
}

//=============================================================================================================

FsAnnotationSet::FsAnnotationSet(const FsAnnotation& p_LHAnnotation, const FsAnnotation& p_RHAnnotation)
{
    if(p_LHAnnotation.hemi() == 0)
        m_qMapAnnots.insert(0, p_LHAnnotation);
    else
        qWarning("Left hemisphere id is not 0. LH annotation not assigned!");

    if(p_RHAnnotation.hemi() == 1)
        m_qMapAnnots.insert(1, p_RHAnnotation);
    else
        qWarning("Right hemisphere id is not 1. RH annotation not assigned!");
}

//=============================================================================================================

FsAnnotationSet::FsAnnotationSet(const QString& p_sLHFileName, const QString& p_sRHFileName)
{
    FsAnnotationSet t_AnnotationSet;
    if(FsAnnotationSet::read(p_sLHFileName, p_sRHFileName, t_AnnotationSet))
        *this = t_AnnotationSet;
}

//=============================================================================================================

void FsAnnotationSet::clear()
{
    m_qMapAnnots.clear();
}

//=============================================================================================================

void FsAnnotationSet::insert(const FsAnnotation& p_Annotation)
{
    if(p_Annotation.isEmpty())
        return;

    qint32 hemi = p_Annotation.hemi();
    m_qMapAnnots.remove(hemi);

    m_qMapAnnots.insert(hemi, p_Annotation);
}

//=============================================================================================================

bool FsAnnotationSet::read(const QString& p_sLHFileName, const QString& p_sRHFileName, FsAnnotationSet &p_AnnotationSet)
{
    p_AnnotationSet.clear();

    QStringList t_qListFileName;
    t_qListFileName << p_sLHFileName << p_sRHFileName;

    for(qint32 i = 0; i < t_qListFileName.size(); ++i)
    {
        FsAnnotation t_Annotation;
        if(FsAnnotation::read(t_qListFileName[i], t_Annotation))
        {
            if(t_qListFileName[i].contains("lh."))
                p_AnnotationSet.m_qMapAnnots.insert(0, t_Annotation);
            else if(t_qListFileName[i].contains("rh."))
                p_AnnotationSet.m_qMapAnnots.insert(1, t_Annotation);
            else
                return false;
        }
    }

    if(p_AnnotationSet.m_qMapAnnots.isEmpty())
        return false;

    return true;
}

//=============================================================================================================

bool FsAnnotationSet::toLabels(const FsSurfaceSet &p_surfSet,
                             QList<FsLabel> &p_qListLabels,
                             QList<RowVector4i> &p_qListLabelRGBAs,
                             const QStringList& lLabelPicks) const
{
    if(!m_qMapAnnots[0].toLabels(p_surfSet[0], p_qListLabels, p_qListLabelRGBAs, lLabelPicks))
        return false;
    else if(!m_qMapAnnots[1].toLabels(p_surfSet[1], p_qListLabels, p_qListLabelRGBAs, lLabelPicks))
        return false;

    return true;
}

//=============================================================================================================

FsAnnotation& FsAnnotationSet::operator[] (qint32 idx)
{
    if(idx == 0)
        return m_qMapAnnots[idx];
    else if(idx == 1)
        return m_qMapAnnots[idx];
    else
    {
        qWarning("Warning: Index is not '0' or '1'! Returning '0'.");
        return m_qMapAnnots[0];
    }
}

//=============================================================================================================

const FsAnnotation FsAnnotationSet::operator[] (qint32 idx) const
{
    if(idx == 0)
        return m_qMapAnnots[idx];
    else if(idx == 1)
        return m_qMapAnnots[idx];
    else
    {
        qWarning("Warning: Index is not '0' or '1'! Returning '0'.");
        return m_qMapAnnots[0];
    }
}

//=============================================================================================================

FsAnnotation& FsAnnotationSet::operator[] (QString idt)
{
    if(idt.compare("lh") == 0)
        return m_qMapAnnots[0];
    else if(idt.compare("rh") == 0)
        return m_qMapAnnots[1];
    else
    {
        qWarning("Warning: Identifier is not 'lh' or 'rh'! Returning 'lh'.");
        return m_qMapAnnots[0];
    }
}

//=============================================================================================================

const FsAnnotation FsAnnotationSet::operator[] (QString idt) const
{
    if(idt.compare("lh") == 0)
        return m_qMapAnnots[0];
    else if(idt.compare("rh") == 0)
        return m_qMapAnnots[1];
    else
    {
        qWarning("Warning: Identifier is not 'lh' or 'rh'! Returning 'lh'.");
        return m_qMapAnnots[0];
    }
}
