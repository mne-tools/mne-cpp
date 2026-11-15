//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fs_annotationset.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implementation of @ref mne::FsAnnotationSet: pairs lh and rh @ref mne::FsAnnotation for whole-brain parcellations.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_annotationset.h"
#include <base/log.h>
#include "fs_surfaceset.h"


//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FsAnnotationSet::FsAnnotationSet()
{
}

//=============================================================================================================

FsAnnotationSet::FsAnnotationSet(const std::string& subject_id, std::int32_t hemi, const std::string& atlas, const std::filesystem::path& subjects_dir)
{
    FsAnnotation t_Annotation;
    if (hemi == 0 || hemi == 1) {
        if (FsAnnotation::read(subject_id, hemi, atlas, subjects_dir, t_Annotation))
            insert(t_Annotation);
    } else if (hemi == 2) {
        if (FsAnnotation::read(subject_id, 0, atlas, subjects_dir, t_Annotation))
            insert(t_Annotation);
        if (FsAnnotation::read(subject_id, 1, atlas, subjects_dir, t_Annotation))
            insert(t_Annotation);
    }
}

//=============================================================================================================

FsAnnotationSet::FsAnnotationSet(const std::filesystem::path& path, std::int32_t hemi, const std::string& atlas)
{
    FsAnnotation t_Annotation;
    if (hemi == 0 || hemi == 1) {
        if (FsAnnotation::read(path, hemi, atlas, t_Annotation))
            insert(t_Annotation);
    } else if (hemi == 2) {
        if (FsAnnotation::read(path, 0, atlas, t_Annotation))
            insert(t_Annotation);
        if (FsAnnotation::read(path, 1, atlas, t_Annotation))
            insert(t_Annotation);
    }
}

//=============================================================================================================

FsAnnotationSet::FsAnnotationSet(const FsAnnotation& p_LHAnnotation, const FsAnnotation& p_RHAnnotation)
{
    if (p_LHAnnotation.hemi() == 0)
        m_annots.insert_or_assign(0, p_LHAnnotation);
    else
        mne::Log::warning("Left hemisphere id is not 0. LH annotation not assigned!");

    if (p_RHAnnotation.hemi() == 1)
        m_annots.insert_or_assign(1, p_RHAnnotation);
    else
        mne::Log::warning("Right hemisphere id is not 1. RH annotation not assigned!");
}

//=============================================================================================================

FsAnnotationSet::FsAnnotationSet(const std::filesystem::path& p_sLHFileName, const std::filesystem::path& p_sRHFileName)
{
    FsAnnotationSet t_AnnotationSet;
    if (FsAnnotationSet::read(p_sLHFileName, p_sRHFileName, t_AnnotationSet))
        *this = t_AnnotationSet;
}

//=============================================================================================================

void FsAnnotationSet::clear()
{
    m_annots.clear();
}

//=============================================================================================================

void FsAnnotationSet::insert(const FsAnnotation& p_Annotation)
{
    if (p_Annotation.empty())
        return;

    m_annots.insert_or_assign(p_Annotation.hemi(), p_Annotation);
}

//=============================================================================================================

bool FsAnnotationSet::read(const std::filesystem::path& p_sLHFileName, const std::filesystem::path& p_sRHFileName, FsAnnotationSet& p_AnnotationSet)
{
    p_AnnotationSet.clear();

    for (const std::filesystem::path* file : {&p_sLHFileName, &p_sRHFileName}) {
        FsAnnotation t_Annotation;
        if (FsAnnotation::read(*file, t_Annotation))
            p_AnnotationSet.m_annots.insert_or_assign(t_Annotation.hemi(), std::move(t_Annotation));
    }

    if (p_AnnotationSet.m_annots.empty())
        return false;

    return true;
}

//=============================================================================================================

bool FsAnnotationSet::toLabels(const FsSurfaceSet& p_surfSet,
                               std::vector<FsLabel>& p_qListLabels,
                               std::vector<RowVector4i>& p_qListLabelRGBAs,
                               const mne::StringList& lLabelPicks) const
{
    if (!(*this)[0].toLabels(p_surfSet[0], p_qListLabels, p_qListLabelRGBAs, lLabelPicks))
        return false;
    else if (!(*this)[1].toLabels(p_surfSet[1], p_qListLabels, p_qListLabelRGBAs, lLabelPicks))
        return false;

    return true;
}

//=============================================================================================================

namespace
{

int hemiIndex(std::int32_t idx)
{
    if (idx != 0 && idx != 1) {
        mne::Log::warning("Warning: Index is not '0' or '1'! Returning '0'.");
        return 0;
    }
    return idx;
}

int hemiIndex(std::string_view idt)
{
    if (idt == "rh")
        return 1;
    if (idt != "lh")
        mne::Log::warning("Warning: Identifier is not 'lh' or 'rh'! Returning 'lh'.");
    return 0;
}

const FsAnnotation& annotationOrEmpty(const std::map<int, FsAnnotation>& annots, int hemi)
{
    static const FsAnnotation empty;
    const auto it = annots.find(hemi);
    return it != annots.end() ? it->second : empty;
}

} // namespace

//=============================================================================================================

FsAnnotation& FsAnnotationSet::operator[](std::int32_t idx)
{
    return m_annots[hemiIndex(idx)];
}

//=============================================================================================================

const FsAnnotation& FsAnnotationSet::operator[](std::int32_t idx) const
{
    return annotationOrEmpty(m_annots, hemiIndex(idx));
}

//=============================================================================================================

FsAnnotation& FsAnnotationSet::operator[](std::string_view idt)
{
    return m_annots[hemiIndex(idt)];
}

//=============================================================================================================

const FsAnnotation& FsAnnotationSet::operator[](std::string_view idt) const
{
    return annotationOrEmpty(m_annots, hemiIndex(idt));
}
