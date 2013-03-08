//=============================================================================================================
/**
* @file     annotationset.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief     AnnotationSet class implementation
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "annotationset.h"
#include "surfaceset.h"

#include <QFile>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AnnotationSet::AnnotationSet()
{

}


//*************************************************************************************************************

AnnotationSet::AnnotationSet(const Annotation& p_sLHAnnotation, const Annotation& p_sRHAnnotation)
{
    if(p_sLHAnnotation.getHemi() == 0)
        m_qMapAnnots.insert(0, p_sLHAnnotation);
    else
        qWarning("Left hemisphere id is not 0. LH annotation not assigned!");

    if(p_sRHAnnotation.getHemi() == 1)
        m_qMapAnnots.insert(1, p_sRHAnnotation);
    else
        qWarning("Right hemisphere id is not 1. RH annotation not assigned!");

}


//*************************************************************************************************************

AnnotationSet::AnnotationSet(const QString& p_sLHFileName, const QString& p_sRHFileName)
{
    AnnotationSet t_AnnotationSet;
    if(AnnotationSet::read(p_sLHFileName, p_sRHFileName, t_AnnotationSet))
        *this = t_AnnotationSet;
}


//*************************************************************************************************************

void AnnotationSet::clear()
{
    m_qMapAnnots.clear();
}


//*************************************************************************************************************

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


//*************************************************************************************************************

bool AnnotationSet::toLabels(const SurfaceSet &p_surfSet, QList<Label> &p_qListLabels, QList<RowVector4i> &p_qListLabelRGBAs) const
{
    if(!m_qMapAnnots[0].toLabels(p_surfSet[0], p_qListLabels, p_qListLabelRGBAs))
        return false;
    else if(!m_qMapAnnots[1].toLabels(p_surfSet[1], p_qListLabels, p_qListLabelRGBAs))
        return false;

    return true;
}


//*************************************************************************************************************

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


//*************************************************************************************************************

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
