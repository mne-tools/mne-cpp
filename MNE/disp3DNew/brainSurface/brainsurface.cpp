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
: RenderableEntity(parent)
, m_pLeftHemisphere(NULL)
, m_pRightHemisphere(NULL)
{
    init();
}


//*************************************************************************************************************

BrainSurface::BrainSurface(const QString &subject_id, qint32 hemi, const QString &surf, const QString &subjects_dir, QEntity *parent)
: RenderableEntity(parent)
, m_SurfaceSet(subject_id, hemi, surf, subjects_dir)
, m_pLeftHemisphere(NULL)
, m_pRightHemisphere(NULL)
{
    init();
}


//*************************************************************************************************************

BrainSurface::BrainSurface(const QString &subject_id, qint32 hemi, const QString &surf, const QString &atlas, const QString &subjects_dir, QEntity *parent)
: RenderableEntity(parent)
, m_SurfaceSet(subject_id, hemi, surf, subjects_dir)
, m_AnnotationSet(subject_id, hemi, atlas, subjects_dir)
, m_pLeftHemisphere(NULL)
, m_pRightHemisphere(NULL)
{
    init();
}


//*************************************************************************************************************

BrainSurface::BrainSurface(const QString& p_sFile, QEntity *parent)
: RenderableEntity(parent)
, m_pLeftHemisphere(NULL)
, m_pRightHemisphere(NULL)
{
    Surface t_Surf(p_sFile);
    m_SurfaceSet.insert(t_Surf);

    init();
}


//*************************************************************************************************************

BrainSurface::~BrainSurface()
{

}


//*************************************************************************************************************

void BrainSurface::updateActivation(const QList<QColor> &activationRightHemi, const QList<QColor> &activationLeftHemi)
{
    // LEFT HEMISPHERE
    //Find brain mesh as component
    int indexSurfaceMeshLeftHemi;
    QComponentList componentsListLeftHemi = m_pLeftHemisphere->components();
    for(int i = 0; i<componentsListLeftHemi.size(); i++)
        if(componentsListLeftHemi.at(i)->objectName() == "m_pSurfaceMesh")
            indexSurfaceMeshLeftHemi = i;

    BrainSurfaceMesh *meshLeftHemi = (BrainSurfaceMesh*)componentsListLeftHemi.at(indexSurfaceMeshLeftHemi);

    //Update activation
    //meshLeftHemi->updateActivation(activationLeftHemi);

    // RIGHT HEMISPHERE
    //Find brain mesh as component
    int indexSurfaceMeshRightHemi;
    QComponentList componentsListRightHemi = m_pRightHemisphere->components();
    for(int i = 0; i<componentsListRightHemi.size(); i++)
        if(componentsListRightHemi.at(i)->objectName() == "m_pSurfaceMesh")
            indexSurfaceMeshRightHemi = i;

    BrainSurfaceMesh *meshRightHemi = (BrainSurfaceMesh*)componentsListRightHemi.at(indexSurfaceMeshRightHemi);

    //Update activation
    m_qlColors.clear();
    for(int i = 0; i<m_SurfaceSet[1].rr().rows() ; i++)
        if(m_SurfaceSet[1].curv()[i] >= 0)
            m_qlColors<<QColor(255.0, 50.0, 50.0);
        else
            m_qlColors<<QColor(80.0, 60.0, 100.0);

    meshRightHemi->updateActivation(m_qlColors);
}


//*************************************************************************************************************

void BrainSurface::init()
{
    //Create hemispheres and add as childs / 0 -> lh, 1 -> rh
    for(int i = 0; i<m_SurfaceSet.size(); i++) {
        if(m_SurfaceSet[i].hemi() == 0)
            m_pLeftHemisphere = QSharedPointer<BrainHemisphere>(new BrainHemisphere(m_SurfaceSet[i], this));

        if(m_SurfaceSet[i].hemi() == 1)
            m_pRightHemisphere = QSharedPointer<BrainHemisphere>(new BrainHemisphere(m_SurfaceSet[i], this));
    }

    std::cout << "Number of children "<<this->children().size()<<std::endl;

    // Calculate bounding box
    calcBoundingBox();

    // Brain initial surface Transform
    int scale = 300;
    this->scaleTransform()->setScale(scale);

    //Brain initial rotation
    this->rotateTransform()->setAxis(QVector3D(1,0,0));
    this->rotateTransform()->setAngleDeg(-90);
}


//*************************************************************************************************************

void BrainSurface::calcBoundingBox()
{
    QMap<qint32, Surface>::const_iterator it = m_SurfaceSet.data().constBegin();

    QVector3D min(it.value().rr().col(0).minCoeff(), it.value().rr().col(1).minCoeff(), it.value().rr().col(2).minCoeff());
    QVector3D max(it.value().rr().col(0).maxCoeff(), it.value().rr().col(1).maxCoeff(), it.value().rr().col(2).maxCoeff());

    for (it = m_SurfaceSet.data().begin()+1; it != m_SurfaceSet.data().end(); ++it)
    {
        for(qint32 i = 0; i < 3; ++i)
        {
            min[i] = min[i] > it.value().rr().col(i).minCoeff() ? it.value().rr().col(i).minCoeff() : min[i];
            max[i] = max[i] < it.value().rr().col(i).maxCoeff() ? it.value().rr().col(i).maxCoeff() : max[i];
        }
    }
    m_vecBoundingBoxMin = min;
    m_vecBoundingBoxMax = max;

    for(qint32 i = 0; i < 3; ++i)
        m_vecBoundingBoxCenter[i] = (m_vecBoundingBoxMin[i]+m_vecBoundingBoxMax[i])/2.0f;
}
