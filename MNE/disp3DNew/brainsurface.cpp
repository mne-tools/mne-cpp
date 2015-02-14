//=============================================================================================================
/**
* @file     brainsurface.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2015
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
* @brief    Implementation of BrainSurface.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainsurface.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DNEWLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainSurface::BrainSurface(QEntity *parent)
: QEntity(parent)
, m_pLeftHemisphere(NULL)
, m_pRightHemisphere(NULL)
{
    init();
}


//*************************************************************************************************************

BrainSurface::BrainSurface(const QString &subject_id, qint32 hemi, const QString &surf, const QString &subjects_dir, QEntity *parent)
: QEntity(parent)
, m_SurfaceSet(subject_id, hemi, surf, subjects_dir)
, m_pLeftHemisphere(NULL)
, m_pRightHemisphere(NULL)
{
    init();
}


//*************************************************************************************************************

BrainSurface::BrainSurface(const QString &subject_id, qint32 hemi, const QString &surf, const QString &atlas, const QString &subjects_dir, QEntity *parent)
: QEntity(parent)
, m_SurfaceSet(subject_id, hemi, surf, subjects_dir)
, m_AnnotationSet(subject_id, hemi, atlas, subjects_dir)
, m_pLeftHemisphere(NULL)
, m_pRightHemisphere(NULL)
{
    init();
}


//*************************************************************************************************************

BrainSurface::BrainSurface(const QString& p_sFile, QEntity *parent)
: QEntity(parent)
, m_pLeftHemisphere(NULL)
, m_pRightHemisphere(NULL)
{
    Surface t_Surf(p_sFile);
    m_SurfaceSet.insert(t_Surf);

    init();
}


//*************************************************************************************************************

void BrainSurface::init()
{
    //Create hemispheres and add as childs / 0 -> lh, 1 -> rh
    for(int i = 0; i<m_SurfaceSet.size(); i++) {
        if(m_SurfaceSet[i].hemi() == 0)
            m_pLeftHemisphere = new Hemisphere(m_SurfaceSet[i], this);

        if(m_SurfaceSet[i].hemi() == 1)
            m_pRightHemisphere = new Hemisphere(m_SurfaceSet[i], this);
    }

    std::cout << "Number of children "<<this->children().size()<<std::endl;

    // Brain surface Transform
    m_pBrainTranslation = new Qt3D::QTranslateTransform();
    m_pBrainRotation = new Qt3D::QRotateTransform();
    m_pBrainTransforms = new Qt3D::QTransform();

//    m_pBrainTranslation->setTranslation(QVector3D(0.0f, 0.0f, 0.0f));
//    m_pBrainRotation->setAxis(QVector3D(1, 0, 0));
//    m_pBrainRotation->setAngleDeg(0.0f);
//    m_pBrainTransforms->addTransform(m_pBrainTranslation);
//    m_pBrainTransforms->addTransform(m_pBrainRotation);

//    this->addComponent(m_pBrainTransforms);
}

