//=============================================================================================================
/**
 * @file     annotationset.cpp
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
 * @brief     AnnotationSet class implementation
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "annotationset.h"
#include "surfaceset.h"

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

AnnotationSet::AnnotationSet()
{
}

//=============================================================================================================

AnnotationSet::AnnotationSet(const QString &subject_id, qint32 hemi, const QString &atlas, const QString &subjects_dir)
{
    Annotation t_Annotation;
    if(hemi == 0 || hemi == 1)
    {
        if(Annotation::read(subject_id, hemi, atlas, subjects_dir, t_Annotation))
            insert(t_Annotation);
    }
    else if(hemi == 2)
    {
        if(Annotation::read(subject_id, 0, atlas, subjects_dir, t_Annotation))
            insert(t_Annotation);
        if(Annotation::read(subject_id, 1, atlas, subjects_dir, t_Annotation))
            insert(t_Annotation);
    }
}

//=============================================================================================================

AnnotationSet::AnnotationSet(const QString &path, qint32 hemi, const QString &atlas)
{
    Annotation t_Annotation;
    if(hemi == 0 || hemi == 1)
    {
        if(Annotation::read(path, hemi, atlas, t_Annotation))
            insert(t_Annotation);
    }
    else if(hemi == 2)
    {
        if(Annotation::read(path, 0, atlas, t_Annotation))
            insert(t_Annotation);
        if(Annotation::read(path, 1, atlas, t_Annotation))
            insert(t_Annotation);
    }
}

//=============================================================================================================

AnnotationSet::AnnotationSet(const Annotation& p_LHAnnotation, const Annotation& p_RHAnnotation)
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

AnnotationSet::AnnotationSet(const QString& p_sLHFileName, const QString& p_sRHFileName)
{
    AnnotationSet t_AnnotationSet;
    if(AnnotationSet::read(p_sLHFileName, p_sRHFileName, t_AnnotationSet))
        *this = t_AnnotationSet;
}

//=============================================================================================================

void AnnotationSet::clear()
{
    m_qMapAnnots.clear();
}

//=============================================================================================================

void AnnotationSet::insert(const Annotation& p_Annotation)
{
    if(p_Annotation.isEmpty())
        return;

    qint32 hemi = p_Annotation.hemi();
    m_qMapAnnots.remove(hemi);

    m_qMapAnnots.insert(hemi, p_Annotation);
}

//=============================================================================================================

bool AnnotationSet::read(const QString& p_sLHFileName, const QString& p_sRHFileName, AnnotationSet &p_AnnotationSet)
{
    p_AnnotationSet.clear();

    QStringList t_qListFileName;
    t_qListFileName << p_sLHFileName << p_sRHFileName;

    for(qint32 i = 0; i < t_qListFileName.size(); ++i)
    {
        Annotation t_Annotation;
        if(Annotation::read(t_qListFileName[i], t_Annotation))
        {
            if(t_qListFileName[i].contains("lh."))
                p_AnnotationSet.m_qMapAnnots.insert(0, t_Annotation);
            else if(t_qListFileName[i].contains("rh."))
                p_AnnotationSet.m_qMapAnnots.insert(1, t_Annotation);
            else
                return false;
        }
    }

    return true;
}

//=============================================================================================================

bool AnnotationSet::toLabels(const SurfaceSet &p_surfSet,
                             QList<Label> &p_qListLabels,
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

Annotation& AnnotationSet::operator[] (qint32 idx)
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

const Annotation AnnotationSet::operator[] (qint32 idx) const
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

Annotation& AnnotationSet::operator[] (QString idt)
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

const Annotation AnnotationSet::operator[] (QString idt) const
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
