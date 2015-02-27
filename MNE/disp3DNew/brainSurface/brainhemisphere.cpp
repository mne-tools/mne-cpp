//=============================================================================================================
/**
* @file     hemisphere.cpp
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
* @brief    Implementation of Hemisphere.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainhemisphere.h"


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

BrainHemisphere::BrainHemisphere(QNode *parent)
: RenderableEntity(parent)
{
    init();
}


//*************************************************************************************************************

BrainHemisphere::BrainHemisphere(const Surface &surf, QNode *parent)
: RenderableEntity(parent)
, m_surface(surf)
{
    init();
}


//*************************************************************************************************************

void BrainHemisphere::init()
{
    //Init colors
    m_qlColors.clear();
    for(int i = 0; i<m_surface.rr().rows() ; i++)
        if(m_surface.curv()[i] >= 0)
            m_qlColors<<QColor(50.0, 50.0, 50.0);
        else
            m_qlColors<<QColor(100.0, 100.0, 100.0);

    //Create mesh for left hemisphere
    m_pSurfaceMesh = QSharedPointer<BrainSurfaceMesh>(new BrainSurfaceMesh(m_surface, m_qlColors));
    m_pSurfaceMesh->setObjectName("m_pSurfaceMesh");

    //this->setMesh(m_pSurfaceMesh.data());
    this->addComponent(m_pSurfaceMesh.data());

    //Translate to the right if this hemisphere is right hemisphere and is inflated
    if(m_surface.surf() == "inflated" && m_surface.hemi() == 1)
        this->translateTransform()->setDx(this->scaleTransform()->scale()/10);

    //Set material
    QVertexMaterial *qVertexMaterial = new QVertexMaterial();
    this->addComponent(qVertexMaterial);

//    QPhongMaterial *phongMaterial = new QPhongMaterial();
//    phongMaterial->setDiffuse(QColor(40, 40, 40));
//    phongMaterial->setAmbient(Qt::gray);
//    phongMaterial->setSpecular(Qt::white);
//    phongMaterial->setShininess(50.0f);
//    this->addComponent(phongMaterial);

//    QDiffuseMapMaterial *diffuseMapMaterial = new QDiffuseMapMaterial();
//    diffuseMapMaterial->setAmbient(Qt::gray);
//    diffuseMapMaterial->setSpecular(Qt::white);
//    diffuseMapMaterial->setShininess(5.0f);
//    this->addComponent(diffuseMapMaterial);
}

