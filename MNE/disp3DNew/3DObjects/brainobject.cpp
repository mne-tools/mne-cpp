//=============================================================================================================
/**
* @file     brainobject.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     November, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    BrainObject class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainobject.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DNEWLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainObject::BrainObject(const Surface &tSurface, float initScale, Qt3DCore::QEntity *parent)
: Renderable3DEntity(tSurface.rr(), tSurface.nn(), tSurface.tris(), -tSurface.offset(), initScale, parent)
, m_sFilePath(tSurface.filePath())
, m_sFileName(tSurface.fileName())
, m_iHemi(tSurface.hemi())
, m_sSurf(tSurface.surf())
, m_vecCurv(tSurface.curv())
, m_vecOffset(tSurface.offset())
, m_ColorSulci(QColor(255,0,0))
, m_ColorGyri(QColor(0,255,0))
, m_matVert(tSurface.rr())
, m_matTris(tSurface.tris())
, m_matNorm(tSurface.nn())
{
    //Create color from curvature information and refresh renderable 3D entity
    m_matColorsOrig.resize(m_matVert.rows(), m_matVert.cols());

    for(int i = 0; i<m_matVert.rows() ; i++) {
        if(m_vecCurv[i] >= 0) {
            m_matColorsOrig(i, 0) = m_ColorSulci.red();
            m_matColorsOrig(i, 1) = m_ColorSulci.green();
            m_matColorsOrig(i, 2) = m_ColorSulci.blue();
        } else {
            m_matColorsOrig(i, 0) = m_ColorGyri.red();
            m_matColorsOrig(i, 1) = m_ColorGyri.green();
            m_matColorsOrig(i, 2) = m_ColorGyri.blue();
        }
    }

    //this->updateVertColors(m_matColorsOrig);
}


//*************************************************************************************************************

BrainObject::~BrainObject()
{
}


//*************************************************************************************************************




