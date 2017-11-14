//=============================================================================================================
/**
* @file     cshinterpolationitem.cpp
* @author   Lars Debor <lars.debor@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     October, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lars Debor and Matti Hamalainen. All rights reserved.
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
* @brief    CshInterpolationItem class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "cshinterpolationitem.h"
#include "../../materials/cshinterpolationmaterial.h"
#include "../../3dhelpers/custommesh.h"
#include <mne/mne_bem_surface.h>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>
#include <Qt3DRender/QComputeCommand>
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QGeometryRenderer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace Qt3DRender;
using namespace Qt3DCore;

//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CshInterpolationItem::CshInterpolationItem(Qt3DCore::QEntity *p3DEntityParent, int iType, const QString &text)
    : Abstract3DTreeItem(p3DEntityParent, iType, text)
    , m_bIsDataInit(false)
    , m_pMaterial(new CshInterpolationMaterial)
{
    initItem();
}


//*************************************************************************************************************

void CshInterpolationItem::initData(const MNELIB::MNEBemSurface &tMneBemSurface,
                                   QSharedPointer<SparseMatrix<double> > tInterpolationMatrix)
{
    if(m_bIsDataInit == true)
    {
       qDebug("CshInterpolationItem::initData data already initialized");
       return;
    }

    m_pMaterial->setWeightMatrix(tInterpolationMatrix);

    //Create draw entity if needed
    if(!m_pMeshDrawEntity)
    {
        m_pMeshDrawEntity = new QEntity(this);

        m_pCustomMesh = new CustomMesh;

        //Interpolated signal attribute
        QAttribute *pInterpolatedSignalAttrib = new QAttribute;
        pInterpolatedSignalAttrib->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
        pInterpolatedSignalAttrib->setDataType(Qt3DRender::QAttribute::Float);
        pInterpolatedSignalAttrib->setVertexSize(4);
        pInterpolatedSignalAttrib->setByteOffset(0);
        pInterpolatedSignalAttrib->setByteStride(4 * sizeof(float));
        pInterpolatedSignalAttrib->setName(QStringLiteral("OutputColor"));
        pInterpolatedSignalAttrib->setBuffer(m_pMaterial->getOutputColorBuffer());

        //add interpolated signal Attribute
        m_pCustomMesh->addAttribute(pInterpolatedSignalAttrib);

        m_pMeshDrawEntity->addComponent(m_pCustomMesh);

        m_pMeshDrawEntity->addComponent(m_pMaterial);
    }

    //Create compute entity if needed
    if(!m_pComputeEntity)
    {
        m_pComputeEntity = new QEntity(this);

        m_pComputeCommand = new QComputeCommand;
        m_pComputeEntity->addComponent(m_pComputeCommand);

        m_pComputeEntity->addComponent(m_pMaterial);
    }

    const uint iWeightMatRows = tMneBemSurface.rr.rows();

    //Set work group size
    const uint iWorkGroupsSize = static_cast<uint>(std::ceil(std::sqrt(iWeightMatRows)));

    m_pComputeCommand->setWorkGroupX(iWorkGroupsSize);
    m_pComputeCommand->setWorkGroupY(iWorkGroupsSize);
    m_pComputeCommand->setWorkGroupZ(1);

    //Set custom mesh data
    //generate mesh base color
    QColor baseColor = QColor(80, 80, 80, 255);
    MatrixX3f matVertColor(tMneBemSurface.rr.rows(),3);

    for(int i = 0; i < matVertColor.rows(); ++i) {
        matVertColor(i,0) = baseColor.redF();
        matVertColor(i,1) = baseColor.greenF();
        matVertColor(i,2) = baseColor.blueF();
    }

    //Set renderable 3D entity mesh and color data
    m_pCustomMesh->setMeshData(tMneBemSurface.rr,
                                tMneBemSurface.nn,
                                tMneBemSurface.tris,
                                matVertColor,
                                Qt3DRender::QGeometryRenderer::Triangles);

    m_bIsDataInit = true;
}


//*************************************************************************************************************

void CshInterpolationItem::setWeightMatrix(QSharedPointer<SparseMatrix<double> > tInterpolationMatrix)
{
    if(m_bIsDataInit == false)
    {
        qDebug("CshInterpolationItem::setWeightMatrix item data is not initialized!");
        return;
    }
    m_pMaterial->setWeightMatrix(tInterpolationMatrix);
}


//*************************************************************************************************************

void CshInterpolationItem::addNewRtData(const VectorXf &tSignalVec)
{
    if(m_pMaterial && m_bIsDataInit)
    {
        m_pMaterial->addSignalData(tSignalVec);
    }

}


//*************************************************************************************************************

void CshInterpolationItem::setNormalization(const QVector3D &tVecThresholds)
{
    m_pMaterial->setNormalization(tVecThresholds);
}


//*************************************************************************************************************

void CshInterpolationItem::setColormapType(const QString &tColormapType)
{
    m_pMaterial->setColormapType(tColormapType);
}


//*************************************************************************************************************

void CshInterpolationItem::initItem()
{
    this->setEditable(false);
    this->setCheckable(true);
    this->setCheckState(Qt::Checked);
    this->setToolTip(this->text());
}


//*************************************************************************************************************
