//=============================================================================================================
/**
* @file     surfaceset.cpp
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
* @brief    Implementation of the SurfaceSet class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "surfaceset.h"

#include <QStringList>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SurfaceSet::SurfaceSet()
{

}


//*************************************************************************************************************

SurfaceSet::SurfaceSet(const Surface& p_sLHSurface, const Surface& p_sRHSurface)
{
    if(p_sLHSurface.getHemi() == 0)
        m_qMapSurfs.insert(0, p_sLHSurface);
    else
        qWarning("Left hemisphere id is not 0. LH surface not assigned!");

    if(p_sRHSurface.getHemi() == 1)
        m_qMapSurfs.insert(1, p_sRHSurface);
    else
        qWarning("Right hemisphere id is not 1. RH surface not assigned!");

}


//*************************************************************************************************************

SurfaceSet::SurfaceSet(const QString& p_sLHFileName, const QString& p_sRHFileName)
{
    SurfaceSet t_SurfaceSet;
    if(SurfaceSet::read(p_sLHFileName, p_sRHFileName, t_SurfaceSet))
        *this = t_SurfaceSet;
}


//*************************************************************************************************************

SurfaceSet::~SurfaceSet()
{
}


//*************************************************************************************************************

void SurfaceSet::clear()
{
    m_qMapSurfs.clear();
}


//*************************************************************************************************************

bool SurfaceSet::read(const QString& p_sLHFileName, const QString& p_sRHFileName, SurfaceSet &p_SurfaceSet)
{
    p_SurfaceSet.clear();

    QStringList t_qListFileName;
    t_qListFileName << p_sLHFileName << p_sRHFileName;

    for(qint32 i = 0; i < t_qListFileName.size(); ++i)
    {
        Surface t_Surface;
        if(Surface::read(t_qListFileName[i], t_Surface))
        {
            if(t_qListFileName[i].contains("lh."))
                p_SurfaceSet.m_qMapSurfs.insert(0, t_Surface);
            else if(t_qListFileName[i].contains("rh."))
                p_SurfaceSet.m_qMapSurfs.insert(1, t_Surface);
            else
                return false;
        }
    }

    return true;
}


//*************************************************************************************************************

Surface SurfaceSet::operator[] (qint32 idx) const
{
    if(idx == 0)
        return m_qMapSurfs[idx];
    else if(idx == 1)
        return m_qMapSurfs[idx];
    else
    {
        qWarning("Warning: Index is not '0' or '1'! Returning '0'.");
        return m_qMapSurfs[0];
    }
}


//*************************************************************************************************************

Surface SurfaceSet::operator[] (QString idt) const
{
    if(idt.compare("lh") == 0)
        return m_qMapSurfs[0];
    else if(idt.compare("rh") == 0)
        return m_qMapSurfs[1];
    else
    {
        qWarning("Warning: Identifier is not 'lh' or 'rh'! Returning 'lh'.");
        return m_qMapSurfs[0];
    }
}
