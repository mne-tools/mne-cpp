//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     fs_surfaceset.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     March 2013
 * @brief    Implementation of @ref mne::FsSurfaceSet: bundles the lh + rh @ref mne::FsSurface for one subject and surface kind.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_surfaceset.h"
#include <base/log.h>
#include <base/string_utils.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FsSurfaceSet::FsSurfaceSet()
{
}

//=============================================================================================================

FsSurfaceSet::FsSurfaceSet(const std::string& subject_id, std::int32_t hemi, const std::string& surf, const std::filesystem::path& subjects_dir)
{
    FsSurface t_Surface;
    if (hemi == 0 || hemi == 1) {
        if (FsSurface::read(subject_id, hemi, surf, subjects_dir, t_Surface))
            insert(t_Surface);
    } else if (hemi == 2) {
        if (FsSurface::read(subject_id, 0, surf, subjects_dir, t_Surface))
            insert(t_Surface);
        if (FsSurface::read(subject_id, 1, surf, subjects_dir, t_Surface))
            insert(t_Surface);
    }

    calcOffset();
}

//=============================================================================================================

FsSurfaceSet::FsSurfaceSet(const std::filesystem::path& path, std::int32_t hemi, const std::string& surf)
{
    FsSurface t_Surface;
    if (hemi == 0 || hemi == 1) {
        if (FsSurface::read(path, hemi, surf, t_Surface))
            insert(t_Surface);
    } else if (hemi == 2) {
        if (FsSurface::read(path, 0, surf, t_Surface))
            insert(t_Surface);
        if (FsSurface::read(path, 1, surf, t_Surface))
            insert(t_Surface);
    }

    calcOffset();
}

//=============================================================================================================

FsSurfaceSet::FsSurfaceSet(const FsSurface& p_LHSurface, const FsSurface& p_RHSurface)
{
    if (p_LHSurface.hemi() == 0)
        m_surfs.insert_or_assign(0, p_LHSurface);
    else
        mne::Log::warning("Left hemisphere id is not 0. LH surface not assigned!");

    if (p_RHSurface.hemi() == 1)
        m_surfs.insert_or_assign(1, p_RHSurface);
    else
        mne::Log::warning("Right hemisphere id is not 1. RH surface not assigned!");

    calcOffset();
}

//=============================================================================================================

FsSurfaceSet::FsSurfaceSet(const std::filesystem::path& p_sLHFileName, const std::filesystem::path& p_sRHFileName)
{
    FsSurfaceSet t_SurfaceSet;
    if (FsSurfaceSet::read(p_sLHFileName, p_sRHFileName, t_SurfaceSet))
        *this = t_SurfaceSet;
}

//=============================================================================================================

FsSurfaceSet::~FsSurfaceSet()
{
}

//=============================================================================================================

void FsSurfaceSet::clear()
{
    m_surfs.clear();
}

//=============================================================================================================

void FsSurfaceSet::insert(const FsSurface& p_Surface)
{
    if (p_Surface.empty())
        return;

    m_surfs.insert_or_assign(p_Surface.hemi(), p_Surface);
}

//=============================================================================================================

bool FsSurfaceSet::read(const std::filesystem::path& p_sLHFileName, const std::filesystem::path& p_sRHFileName, FsSurfaceSet& p_SurfaceSet)
{
    p_SurfaceSet.clear();

    for (const std::filesystem::path* file : {&p_sLHFileName, &p_sRHFileName}) {
        FsSurface t_Surface;
        if (FsSurface::read(*file, t_Surface))
            p_SurfaceSet.m_surfs.insert_or_assign(t_Surface.hemi(), std::move(t_Surface));
    }

    if (p_SurfaceSet.m_surfs.empty())
        return false;

    p_SurfaceSet.calcOffset();

    return true;
}

//=============================================================================================================

namespace
{

std::int32_t hemiIndex(std::int32_t idx)
{
    if (idx != 0 && idx != 1) {
        mne::Log::warning("Warning: Index is not '0' or '1'! Returning '0'.");
        return 0;
    }
    return idx;
}

std::int32_t hemiIndex(std::string_view idt)
{
    if (idt == "rh")
        return 1;
    if (idt != "lh")
        mne::Log::warning("Warning: Identifier is not 'lh' or 'rh'! Returning 'lh'.");
    return 0;
}

const FsSurface& surfaceOrEmpty(const std::map<int, FsSurface>& surfs, std::int32_t hemi)
{
    static const FsSurface empty;
    const auto it = surfs.find(hemi);
    return it != surfs.end() ? it->second : empty;
}

} // namespace

//=============================================================================================================

const FsSurface& FsSurfaceSet::operator[](std::int32_t idx) const
{
    return surfaceOrEmpty(m_surfs, hemiIndex(idx));
}

//=============================================================================================================

FsSurface& FsSurfaceSet::operator[](std::int32_t idx)
{
    return m_surfs[hemiIndex(idx)];
}

//=============================================================================================================

const FsSurface& FsSurfaceSet::operator[](std::string_view idt) const
{
    return surfaceOrEmpty(m_surfs, hemiIndex(idt));
}

//=============================================================================================================

FsSurface& FsSurfaceSet::operator[](std::string_view idt)
{
    return m_surfs[hemiIndex(idt)];
}

//=============================================================================================================

void FsSurfaceSet::calcOffset()
{
    //
    // Correct inflated offset
    //
    if (m_surfs.size() == 2 && m_surfs.begin()->second.surf() == "inflated") {
        float xOffset = m_surfs.at(0).rr().col(0).maxCoeff() - m_surfs.at(1).rr().col(0).minCoeff();
        Vector3f vecLhOffset, vecRhOffset;
        vecLhOffset << (xOffset / 2.0f), 0, 0;
        vecRhOffset << (-xOffset / 2.0f), 0, 0;
        m_surfs.at(0).offset() = vecLhOffset;
        m_surfs.at(1).offset() = vecRhOffset;
    }
}
