//=============================================================================================================
/**
 * @file     sensorsurfacetreeitem.cpp
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     March, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lars Debor, Lorenz Esch. All rights reserved.
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
 * @brief    SensorSurfaceTreeItem class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sensorsurfacetreeitem.h"
#include "../common/metatreeitem.h"
#include "../../3dhelpers/renderable3Dentity.h"
#include "../../materials/pervertexphongalphamaterial.h"
#include "../../3dhelpers/custommesh.h"

#include <mne/mne_bem.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace Eigen;
using namespace MNELIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SensorSurfaceTreeItem::SensorSurfaceTreeItem(Qt3DCore::QEntity *p3DEntityParent, int iType, const QString& text)
: AbstractMeshTreeItem(p3DEntityParent, iType, text)
{
    initItem();
}

//=============================================================================================================

void SensorSurfaceTreeItem::initItem()
{
    this->setEditable(false);
    this->setCheckable(true);
    this->setCheckState(Qt::Checked);
    this->setToolTip("Sensor surface item");

    //Set material to enable sorting
    PerVertexPhongAlphaMaterial* pBemMaterial = new PerVertexPhongAlphaMaterial(true);
    this->setMaterial(pBemMaterial);
}

//=============================================================================================================

void SensorSurfaceTreeItem::addData(const MNEBemSurface& tSensorSurface)
{
    //Create color from curvature information with default gyri and sulcus colors
    MatrixX4f matVertColor = createVertColor(tSensorSurface.rr.rows());

    //Set renderable 3D entity mesh and color data
    m_pCustomMesh->setMeshData(tSensorSurface.rr,
                               tSensorSurface.nn,
                               tSensorSurface.tris,
                               matVertColor,
                               Qt3DRender::QGeometryRenderer::Triangles);

    //Add data which is held by this SensorSurfaceTreeItem
    QVariant data;

    data.setValue(tSensorSurface.rr.rows());
    this->setData(data, Data3DTreeModelItemRoles::NumberVertices);
}

