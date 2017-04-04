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
#include "../common/metatreeitem.h"

#include <fiff/fiff_constants.h>
#include <fiff/fiff_dig_point.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <Qt3DExtras/QSphereMesh>
#include <Qt3DExtras/QPhongMaterial>


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

DigitizerTreeItem::DigitizerTreeItem(Qt3DCore::QEntity *p3DEntityParent, int iType, const QString& text)
: Abstract3DTreeItem(p3DEntityParent, iType, text)
{
    initItem();
}


//*************************************************************************************************************

void DigitizerTreeItem::initItem()
{
    this->setEditable(false);
    this->setCheckable(true);
    this->setCheckState(Qt::Checked);
    this->setToolTip(this->text());

    //Add color picker item as meta information item
    QVariant data;
    QList<QStandardItem*> list;

    MetaTreeItem* pItemColor = new MetaTreeItem(MetaTreeItemTypes::Color, "Point color");
    connect(pItemColor, &MetaTreeItem::dataChanged,
            this, &DigitizerTreeItem::onColorChanged);
    list.clear();
    list << pItemColor;
    list << new QStandardItem(pItemColor->toolTip());
    this->appendRow(list);
    data.setValue(QColor(100,100,100));
    pItemColor->setData(data, MetaTreeItemRoles::Color);
    pItemColor->setData(data, Qt::DecorationRole);
}


//*************************************************************************************************************

void DigitizerTreeItem::addData(const QList<FIFFLIB::FiffDigPoint>& tDigitizer)
{
    //Create digitizers as small 3D spheres
    QVector3D pos;
    QColor colDefault(100,100,100);

    for(int i = 0; i < tDigitizer.size(); ++i) {
        Renderable3DEntity* pSourceSphereEntity = new Renderable3DEntity(this);

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
    }

    //Update colors in color item
    QList<QStandardItem*> items = this->findChildren(MetaTreeItemTypes::Color);

    for(int i = 0; i < items.size(); ++i) {
        if(MetaTreeItem* item = dynamic_cast<MetaTreeItem*>(items.at(i))) {
            QVariant data;
            data.setValue(colDefault);
            item->setData(data, MetaTreeItemRoles::Color);
            item->setData(data, Qt::DecorationRole);
        }
    }
}


//*************************************************************************************************************

void DigitizerTreeItem::onColorChanged(const QVariant& color)
{
    if(color.canConvert<QColor>()) {
        for(int i = 0; i < this->childNodes().size(); ++i) {
            if(Qt3DCore::QEntity* pNode = dynamic_cast<Qt3DCore::QEntity*>(this->childNodes().at(i))) {
                for(int j = 0; j < pNode->components().size(); ++j) {
                    Qt3DCore::QComponent* pComponent = pNode->components().at(j);

                    if(Qt3DExtras::QPhongMaterial* pMaterial = dynamic_cast<Qt3DExtras::QPhongMaterial*>(pComponent)) {
                        pMaterial->setAmbient(color.value<QColor>());
                    }
                }
            }
        }
    }
}
