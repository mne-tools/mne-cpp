//=============================================================================================================
/**
 * @file     fs_surfaceset.cpp
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
 * @brief    Definition of the FsSurfaceSet class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fs_surfaceset.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QStringList>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FsSurfaceSet::FsSurfaceSet()
{
}

//=============================================================================================================

FsSurfaceSet::FsSurfaceSet(const QString &subject_id, qint32 hemi, const QString &surf, const QString &subjects_dir)
{
    FsSurface t_Surface;
    if(hemi == 0 || hemi == 1)
    {
        if(FsSurface::read(subject_id, hemi, surf, subjects_dir, t_Surface))
            insert(t_Surface);
    }
    else if(hemi == 2)
    {
        if(FsSurface::read(subject_id, 0, surf, subjects_dir, t_Surface))
            insert(t_Surface);
        if(FsSurface::read(subject_id, 1, surf, subjects_dir, t_Surface))
            insert(t_Surface);
    }

    calcOffset();
}

//=============================================================================================================

FsSurfaceSet::FsSurfaceSet(const QString &path, qint32 hemi, const QString &surf)
{
    FsSurface t_Surface;
    if(hemi == 0 || hemi == 1)
    {
        if(FsSurface::read(path, hemi, surf, t_Surface))
            insert(t_Surface);
    }
    else if(hemi == 2)
    {
        if(FsSurface::read(path, 0, surf, t_Surface))
            insert(t_Surface);
        if(FsSurface::read(path, 1, surf, t_Surface))
            insert(t_Surface);
    }

    calcOffset();
}

//=============================================================================================================

FsSurfaceSet::FsSurfaceSet(const FsSurface& p_LHSurface, const FsSurface& p_RHSurface)
{
    if(p_LHSurface.hemi() == 0)
        m_qMapSurfs.insert(0, p_LHSurface);
    else
        qWarning("Left hemisphere id is not 0. LH surface not assigned!");

    if(p_RHSurface.hemi() == 1)
        m_qMapSurfs.insert(1, p_RHSurface);
    else
        qWarning("Right hemisphere id is not 1. RH surface not assigned!");

    calcOffset();
}

//=============================================================================================================

FsSurfaceSet::FsSurfaceSet(const QString& p_sLHFileName, const QString& p_sRHFileName)
{
    FsSurfaceSet t_SurfaceSet;
    if(FsSurfaceSet::read(p_sLHFileName, p_sRHFileName, t_SurfaceSet))
        *this = t_SurfaceSet;
}

//=============================================================================================================

FsSurfaceSet::~FsSurfaceSet()
{
}

//=============================================================================================================

void FsSurfaceSet::clear()
{
    m_qMapSurfs.clear();
}

//=============================================================================================================

void FsSurfaceSet::insert(const FsSurface& p_Surface)
{
    if(p_Surface.isEmpty())
        return;

    qint32 hemi = p_Surface.hemi();
    m_qMapSurfs.remove(hemi);

    m_qMapSurfs.insert(hemi, p_Surface);
}

//=============================================================================================================

bool FsSurfaceSet::read(const QString& p_sLHFileName, const QString& p_sRHFileName, FsSurfaceSet &p_SurfaceSet)
{
    p_SurfaceSet.clear();

    QStringList t_qListFileName;
    t_qListFileName << p_sLHFileName << p_sRHFileName;

    for(qint32 i = 0; i < t_qListFileName.size(); ++i)
    {
        FsSurface t_Surface;
        if(FsSurface::read(t_qListFileName[i], t_Surface))
        {
            if(t_qListFileName[i].contains("lh."))
                p_SurfaceSet.m_qMapSurfs.insert(0, t_Surface);
            else if(t_qListFileName[i].contains("rh."))
                p_SurfaceSet.m_qMapSurfs.insert(1, t_Surface);
            else
                return false;
        }
    }

    if(p_SurfaceSet.m_qMapSurfs.isEmpty())
        return false;

    p_SurfaceSet.calcOffset();

    return true;
}

//=============================================================================================================

const FsSurface& FsSurfaceSet::operator[] (qint32 idx) const
{
    if(idx == 0)
        return m_qMapSurfs.find(idx).value();
    else if(idx == 1)
        return m_qMapSurfs.find(idx).value();
    else
    {
        qWarning("Warning: Index is not '0' or '1'! Returning '0'.");
        return m_qMapSurfs.find(0).value();
    }
}

//=============================================================================================================

FsSurface& FsSurfaceSet::operator[] (qint32 idx)
{
    if(idx == 0)
        return m_qMapSurfs.find(idx).value();
    else if(idx == 1)
        return m_qMapSurfs.find(idx).value();
    else
    {
        qWarning("Warning: Index is not '0' or '1'! Returning '0'.");
        return m_qMapSurfs.find(0).value();
    }
}

//=============================================================================================================

const FsSurface& FsSurfaceSet::operator[] (QString idt) const
{
    if(idt.compare("lh") == 0)
        return m_qMapSurfs.find(0).value();
    else if(idt.compare("rh") == 0)
        return m_qMapSurfs.find(1).value();
    else
    {
        qWarning("Warning: Identifier is not 'lh' or 'rh'! Returning 'lh'.");
        return m_qMapSurfs.find(0).value();
    }
}

//=============================================================================================================

FsSurface& FsSurfaceSet::operator[] (QString idt)
{
    if(idt.compare("lh") == 0)
        return m_qMapSurfs.find(0).value();
    else if(idt.compare("rh") == 0)
        return m_qMapSurfs.find(1).value();
    else
    {
        qWarning("Warning: Identifier is not 'lh' or 'rh'! Returning 'lh'.");
        return m_qMapSurfs.find(0).value();
    }
}

//=============================================================================================================

void FsSurfaceSet::calcOffset()
{
    //
    // Correct inflated offset
    //
    if(m_qMapSurfs.size() == 2 && QString::compare(m_qMapSurfs.begin().value().surf(),"inflated") == 0)
    {
        float xOffset = m_qMapSurfs.find(0).value().rr().col(0).maxCoeff() - m_qMapSurfs.find(1).value().rr().col(0).minCoeff();
        Vector3f vecLhOffset, vecRhOffset;
        vecLhOffset << (xOffset/2.0f), 0, 0;
        vecRhOffset << (-xOffset/2.0f), 0, 0;
        m_qMapSurfs.find(0).value().offset() = vecLhOffset;
        m_qMapSurfs.find(1).value().offset() = vecRhOffset;
    }
}
