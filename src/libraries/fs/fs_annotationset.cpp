//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file fs_annotationset.cpp
 * @since 2026
 * @date  March 2026
 * @brief Implementation of @ref FSLIB::FsAnnotationSet: pairs lh and rh @ref FSLIB::FsAnnotation for whole-brain parcellations.
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
