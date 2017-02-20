//=============================================================================================================
/**
* @file     digitizertreeitem.cpp
* @author   Jana Kiesel <jana.kiesel@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Jana Kiesel and Matti Hamalainen. All rights reserved.
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
* @brief    DigitizerTreeItem class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "digitizertreeitem.h"
#include "../../3dhelpers/renderable3Dentity.h"
#include "../common/metatreeitem.h"

#include <fiff/fiff_constants.h>
#include <fiff/fiff_dig_point.h>


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

DigitizerTreeItem::DigitizerTreeItem(int iType, const QString& text)
: AbstractTreeItem(iType, text)
, m_pRenderable3DEntity(Q_NULLPTR)
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
            this, &DigitizerTreeItem::onSurfaceColorChanged);
    list.clear();
    list << pItemSurfCol;
    list << new QStandardItem(pItemSurfCol->toolTip());
    this->appendRow(list);
    data.setValue(Qt::blue);
    pItemSurfCol->setData(data, MetaTreeItemRoles::PointColor);
    pItemSurfCol->setData(data, Qt::DecorationRole);
}


//*************************************************************************************************************

DigitizerTreeItem::~DigitizerTreeItem()
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

QVariant DigitizerTreeItem::data(int role) const
{
    return AbstractTreeItem::data(role);
}


//*************************************************************************************************************

void  DigitizerTreeItem::setData(const QVariant& value, int role)
{
    AbstractTreeItem::setData(value, role);

    switch(role) {
    case Data3DTreeModelItemRoles::SurfaceCurrentColorVert:
        if(!m_pRenderable3DEntity.isNull()) {
            m_pRenderable3DEntity->setVertColor(value.value<QByteArray>());
        }
        break;
    }
}


//*************************************************************************************************************

bool DigitizerTreeItem::addData(const QList<FIFFLIB::FiffDigPoint>& tDigitizer, Qt3DCore::QEntity* parent)
{
    //Clear all data
    m_lSpheres.clear();

//    if(!m_pRenderable3DEntity.isNull()) {
//        m_pRenderable3DEntity->deleteLater();
//    }

    m_pRenderable3DEntity = new Renderable3DEntity(parent);

    //Create digitizers as small 3D spheres
    QVector3D pos;
    QColor colDefault(100,100,100);

    for(int i = 0; i < tDigitizer.size(); ++i) {
        Renderable3DEntity* pSourceSphereEntity = new Renderable3DEntity(m_pRenderable3DEntity);

        pos.setX(tDigitizer[i].r[0]);
        pos.setY(tDigitizer[i].r[1]);
        pos.setZ(tDigitizer[i].r[2]);

        Qt3DExtras::QSphereMesh* sourceSphere = new Qt3DExtras::QSphereMesh();

        if (tDigitizer[i].kind == FIFFV_POINT_CARDINAL) {
            sourceSphere->setRadius(0.002f);
        } else {
            sourceSphere->setRadius(0.001f);
        }
        pSourceSphereEntity->addComponent(sourceSphere);
        pSourceSphereEntity->setPosition(pos);

        Qt3DExtras::QPhongMaterial* material = new Qt3DExtras::QPhongMaterial();

        switch (tDigitizer[i].kind) {
        case FIFFV_POINT_CARDINAL:
            switch (tDigitizer[i].ident) {
            case 1:
            colDefault = Qt::green;
            material->setAmbient(colDefault);
            break;
            case 2:
            colDefault = Qt::yellow;
            material->setAmbient(colDefault);
            break;
            case 3:
            colDefault = Qt::darkGreen;
            material->setAmbient(colDefault);
            break;
            default:
            colDefault = Qt::white;
            material->setAmbient(colDefault);
            break;
            }
            break;

//            colDefault = Qt::yellow;
//            material->setAmbient(colDefault);
//            break;
        case FIFFV_POINT_HPI:
            colDefault = Qt::red;
            material->setAmbient(colDefault);
            break;
        case FIFFV_POINT_EEG:
            colDefault = Qt::cyan;
            material->setAmbient(colDefault);
            break;
        case FIFFV_POINT_EXTRA:
            colDefault = Qt::magenta;
            material->setAmbient(colDefault);
            break;
        default:
            colDefault = Qt::white;
            material->setAmbient(colDefault);
            break;
        }

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

void DigitizerTreeItem::setVisible(bool state)
{
    if(!m_pRenderable3DEntity.isNull()) {
        for(int i = 0; i < m_lSpheres.size(); ++i) {
            m_lSpheres.at(i)->setEnabled(state);
        }

        m_pRenderable3DEntity->setEnabled(state);
    }
}


//*************************************************************************************************************

void DigitizerTreeItem::onCheckStateChanged(const Qt::CheckState& checkState)
{
    this->setVisible(checkState == Qt::Unchecked ? false : true);
}


//*************************************************************************************************************

void DigitizerTreeItem::onSurfaceColorChanged(const QColor& color)
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
