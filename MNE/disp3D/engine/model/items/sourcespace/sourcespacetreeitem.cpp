//=============================================================================================================
/**
* @file     sourcespacetreeitem.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2016
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
* @brief    SourceSpaceTreeItem class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sourcespacetreeitem.h"
#include "../common/metatreeitem.h"
#include "../../3dhelpers/renderable3Dentity.h"
#include "../../3dhelpers/custommesh.h"

#include <mne/mne_hemisphere.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <Qt3DExtras/QSphereMesh>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DCore/QTransform>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace MNELIB;
using namespace DISP3DLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SourceSpaceTreeItem::SourceSpaceTreeItem(int iType, const QString& text)
: AbstractMeshTreeItem(iType, text)
{
    initItem();
}


//*************************************************************************************************************

void SourceSpaceTreeItem::onCheckStateChanged(const Qt::CheckState& checkState)
{
    this->setVisible(checkState == Qt::Unchecked ? false : true);
}


//*************************************************************************************************************

void SourceSpaceTreeItem::initItem()
{
    this->setEditable(false);
    this->setCheckable(true);
    this->setCheckState(Qt::Checked);
    this->setToolTip("Source space item");
}


//*************************************************************************************************************

void SourceSpaceTreeItem::addData(const MNEHemisphere& tHemisphere, Qt3DCore::QEntity* parent)
{
    //Set parents
    m_pRenderable3DEntity->setParent(parent);

    m_pRenderable3DEntity->setRotX(40);

    //Create color from curvature information with default gyri and sulcus colors
    MatrixX3f matVertColor = createVertColor(tHemisphere.rr);

    //Set renderable 3D entity mesh and color data
    m_pCustomMesh->setMeshData(tHemisphere.rr,
                                tHemisphere.nn,
                                tHemisphere.tris,
                                matVertColor,
                                Qt3DRender::QGeometryRenderer::Triangles);

    //Add data which is held by this SourceSpaceTreeItem
    QVariant data;

    data.setValue(tHemisphere.rr);
    this->setData(data, Data3DTreeModelItemRoles::SurfaceVert);

    plotSources(tHemisphere);
}


//*************************************************************************************************************

void SourceSpaceTreeItem::plotSources(const MNEHemisphere& tHemisphere)
{
    //Create sources as small 3D spheres
    RowVector3f sourcePos;
    QVector3D pos;
    QColor defaultColor(255,0,0);

    if(tHemisphere.isClustered()) {
        for(int i = 0; i < tHemisphere.cluster_info.centroidVertno.size(); i++) {
            Renderable3DEntity* pSourceSphereEntity = new Renderable3DEntity(m_pRenderable3DEntity);

            sourcePos = tHemisphere.rr.row(tHemisphere.cluster_info.centroidVertno.at(i));
            pos.setX(sourcePos(0));
            pos.setY(sourcePos(1));
            pos.setZ(sourcePos(2));

            Qt3DExtras::QSphereMesh* sourceSphere = new Qt3DExtras::QSphereMesh();
            sourceSphere->setRadius(0.001f);
            pSourceSphereEntity->addComponent(sourceSphere);

            pSourceSphereEntity->setPosition(pos);

            Qt3DExtras::QPhongMaterial* material = new Qt3DExtras::QPhongMaterial();
            material->setAmbient(defaultColor);
            pSourceSphereEntity->addComponent(material);

            m_lSpheres.append(pSourceSphereEntity);
        }
    } else {
        for(int i = 0; i < tHemisphere.vertno.rows(); i++) {
            Renderable3DEntity* pSourceSphereEntity = new Renderable3DEntity(m_pRenderable3DEntity);

            sourcePos = tHemisphere.rr.row(tHemisphere.vertno(i));
            pos.setX(sourcePos(0));
            pos.setY(sourcePos(1));
            pos.setZ(sourcePos(2));

            Qt3DExtras::QSphereMesh* sourceSphere = new Qt3DExtras::QSphereMesh();
            sourceSphere->setRadius(0.001f);
            pSourceSphereEntity->addComponent(sourceSphere);

            pSourceSphereEntity->setPosition(pos);

            Qt3DExtras::QPhongMaterial* material = new Qt3DExtras::QPhongMaterial();
            material->setAmbient(defaultColor);
            pSourceSphereEntity->addComponent(material);

            m_lSpheres.append(pSourceSphereEntity);
        }
    }
}


//*************************************************************************************************************

void SourceSpaceTreeItem::setVisible(bool state)
{
    for(int i = 0; i < m_lSpheres.size(); ++i) {
        m_lSpheres.at(i)->setEnabled(state);
    }

    m_pRenderable3DEntity->setEnabled(state);
}
