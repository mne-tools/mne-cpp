//=============================================================================================================
/**
* @file     sensorpositiontreeitem.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     March, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lroenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    SensorPositionTreeItem class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sensorpositiontreeitem.h"
#include "../../3dhelpers/renderable3Dentity.h"
#include "../common/metatreeitem.h"

#include <fiff/fiff_constants.h>
#include <fiff/fiff_ch_info.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QMatrix4x4>
#include <Qt3DExtras/QSphereMesh>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DCore/QTransform>
#include <Qt3DCore/QEntity>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SensorPositionTreeItem::SensorPositionTreeItem(int iType, const QString& text)
: AbstractTreeItem(iType, text)
, m_pRenderable3DEntity(new Renderable3DEntity())
{
    this->setEditable(false);
    this->setCheckable(true);
    this->setCheckState(Qt::Checked);
    this->setToolTip(text);

    //Add color picker item as meta information item
    QVariant data;
    QList<QStandardItem*> list;

    MetaTreeItem* pItemSurfCol = new MetaTreeItem(MetaTreeItemTypes::PointColor, "Point color");
    connect(pItemSurfCol, &MetaTreeItem::surfaceColorChanged,
            this, &SensorPositionTreeItem::onSurfaceColorChanged);
    list.clear();
    list << pItemSurfCol;
    list << new QStandardItem(pItemSurfCol->toolTip());
    this->appendRow(list);
    data.setValue(Qt::blue);
    pItemSurfCol->setData(data, MetaTreeItemRoles::PointColor);
    pItemSurfCol->setData(data, Qt::DecorationRole);
}


//*************************************************************************************************************

SensorPositionTreeItem::~SensorPositionTreeItem()
{
    //Schedule deletion/Decouple of all entities so that the SceneGraph is NOT plotting them anymore.
    for(int i = 0; i < m_lSpheres.size(); ++i) {
        m_lSpheres.at(i)->deleteLater();
    }

    if(!m_pRenderable3DEntity.isNull()) {
        m_pRenderable3DEntity->deleteLater();
    }
}


//*************************************************************************************************************

QVariant SensorPositionTreeItem::data(int role) const
{
    return AbstractTreeItem::data(role);
}


//*************************************************************************************************************

void SensorPositionTreeItem::setData(const QVariant& value, int role)
{
    AbstractTreeItem::setData(value, role);
}


//*************************************************************************************************************

bool SensorPositionTreeItem::addData(const QList<FIFFLIB::FiffChInfo>& lChInfo, Qt3DCore::QEntity* parent)
{
    //Clear all data
    m_lSpheres.clear();

//    if(!m_pRenderable3DEntity.isNull()) {
//        m_pRenderable3DEntity->deleteLater();
//    }

    m_pRenderable3DEntity->setParent(parent);

    //Create digitizers as small 3D spheres
    QVector3D pos;
    QColor colDefault(100,100,100);

    for(int i = 0; i < lChInfo.size(); ++i) {
        Renderable3DEntity* pSourceSphereEntity = new Renderable3DEntity(m_pRenderable3DEntity);

        pos.setX(lChInfo[i].chpos.r0(0));
        pos.setY(lChInfo[i].chpos.r0(1));
        pos.setZ(lChInfo[i].chpos.r0(2));

        Qt3DExtras::QSphereMesh* sourceSphere = new Qt3DExtras::QSphereMesh();

        pSourceSphereEntity->addComponent(sourceSphere);
        pSourceSphereEntity->setPosition(pos);

        Qt3DExtras::QPhongMaterial* material = new Qt3DExtras::QPhongMaterial();

        colDefault = Qt::green;

        pSourceSphereEntity->addComponent(material);

        pSourceSphereEntity->setParent(m_pRenderable3DEntity);

        m_lSpheres.append(pSourceSphereEntity);
    }

    QList<QStandardItem*> items = this->findChildren(MetaTreeItemTypes::PointColor);

    for(int i = 0; i < items.size(); ++i) {
        if(MetaTreeItem* item = dynamic_cast<MetaTreeItem*>(items.at(i))) {
            QVariant data;
            data.setValue(colDefault);
            item->setData(data, MetaTreeItemRoles::PointColor);
        }
    }

    return true;
}


//*************************************************************************************************************

void SensorPositionTreeItem::setVisible(bool state)
{
    if(!m_pRenderable3DEntity.isNull()) {
        for(int i = 0; i < m_lSpheres.size(); ++i) {
            m_lSpheres.at(i)->setEnabled(state);
        }

        m_pRenderable3DEntity->setEnabled(state);
    }
}


//*************************************************************************************************************

void SensorPositionTreeItem::onCheckStateChanged(const Qt::CheckState& checkState)
{
    this->setVisible(checkState == Qt::Unchecked ? false : true);
}


//*************************************************************************************************************

void SensorPositionTreeItem::onSurfaceColorChanged(const QColor& color)
{
    for(int i = 0; i < m_lSpheres.size(); ++i) {
        for(int j = 0; j < m_lSpheres.at(i)->components().size(); ++j) {
            Qt3DCore::QComponent* pComponent = m_lSpheres.at(i)->components().at(j);

            if(Qt3DExtras::QPhongMaterial* pMaterial = dynamic_cast<Qt3DExtras::QPhongMaterial*>(pComponent)) {
                pMaterial->setAmbient(color);
            }
        }
    }
}
