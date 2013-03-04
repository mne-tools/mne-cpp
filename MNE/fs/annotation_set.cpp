//=============================================================================================================
/**
* @file     annotation_set.cpp
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

#include "annotation_set.h"

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
    src_annotations.insert(0, p_sLHAnnotation);
    src_annotations.insert(1, p_sRHAnnotation);
}


//*************************************************************************************************************

AnnotationSet::AnnotationSet(const QString& p_sLHFileName, const QString& p_sRHFileName)
{
    QStringList t_qListFileName;
    t_qListFileName << p_sLHFileName << p_sRHFileName;

    AnnotationSet t_AnnotationSet;
    if(AnnotationSet::read(t_qListFileName, t_AnnotationSet))
        *this = t_AnnotationSet;
}


//*************************************************************************************************************

void AnnotationSet::clear()
{
    src_annotations.clear();
}


//*************************************************************************************************************

bool AnnotationSet::read(const QStringList &p_qListFileNames, AnnotationSet &p_AnnotationSet)
{
    p_AnnotationSet.clear();

    for(qint32 i = 0; i < p_qListFileNames.size(); ++i)
    {
        Annotation t_Annotation;
        if(Annotation::read(p_qListFileNames[i], t_Annotation))
        {
            if(p_qListFileNames[i].contains("lh."))
                p_AnnotationSet.src_annotations.insert(0, t_Annotation);
            else if(p_qListFileNames[i].contains("rh."))
                p_AnnotationSet.src_annotations.insert(1, t_Annotation);
            else
                return false;
        }
    }

    return true;
}


//*************************************************************************************************************

Annotation& AnnotationSet::operator[] (qint32 idx)
{
    if(src_annotations.size() > idx)
        return src_annotations[idx];
    else
    {
        qWarning("Warning: Index out of bound! Returning last element.");
        return src_annotations[src_annotations.size()-1];
    }
}


//*************************************************************************************************************

Annotation& AnnotationSet::operator[] (QString idt)
{
    if(idt.compare("lh") == 0)
        return src_annotations[0];
    else if(idt.compare("rh") == 0)
        return src_annotations[1];
    else
    {
        qWarning("Warning: Identifier is not 'lh' or 'rh'! Returning 'lh'.");
        return src_annotations[0];
    }
}
