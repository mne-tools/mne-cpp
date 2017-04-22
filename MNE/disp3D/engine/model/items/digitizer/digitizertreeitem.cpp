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
#include <Qt3DExtras/QPhongAlphaMaterial>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//============================================================================= ================================


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
}


//*************************************************************************************************************

void DigitizerTreeItem::addData(const QList<FIFFLIB::FiffDigPoint>& tDigitizer)
{
    QVector3D pos;

    if(!list.isEmpty() && list.size() == tDigitizer.size()) {
        for(int i = 0; i < list.size(); ++i) {
            pos.setX(tDigitizer[i].r[0]);
            pos.setY(tDigitizer[i].r[1]);
            pos.setZ(tDigitizer[i].r[2]);

            list[i]->setPosition(pos);
        }
        return;
    }

//    for(int i = 0; i < list.size(); ++i) {
//        list[i]->deleteLater();
//    }
//    list.clear();

    //Create digitizers as small 3D spheres
    QColor colDefault(100,100,100);

    for(int i = 0; i < tDigitizer.size(); ++i) {
        Renderable3DEntity* pSourceSphereEntity = new Renderable3DEntity(this);

        list.append(pSourceSphereEntity);
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

        Qt3DExtras::QPhongAlphaMaterial* material = new Qt3DExtras::QPhongAlphaMaterial();

        switch (tDigitizer[i].kind) {
            case FIFFV_POINT_CARDINAL:
                switch (tDigitizer[i].ident) {
                    case FIFFV_POINT_LPA:
                    colDefault = Qt::green;
                    material->setAmbient(colDefault);
                    break;
                    case FIFFV_POINT_NASION:
                    colDefault = Qt::yellow;
                    material->setAmbient(colDefault);
                    break;
                    case FIFFV_POINT_RPA:
                    colDefault = Qt::magenta;
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

    //Update alpha
    float alpha = 1.0;
    this->setMaterialParameter(QVariant(alpha), "alpha");

    QList<QStandardItem*> items = m_pItemAppearanceOptions->findChildren(MetaTreeItemTypes::AlphaValue);

    for(int i = 0; i < items.size(); ++i) {
        if(MetaTreeItem* item = dynamic_cast<MetaTreeItem*>(items.at(i))) {
            QVariant data;
            data.setValue(alpha);
            item->setData(data, MetaTreeItemRoles::AlphaValue);
            item->setData(data, Qt::DecorationRole);
        }
    }

    //Update colors in color item
    items = m_pItemAppearanceOptions->findChildren(MetaTreeItemTypes::Color);

    for(int i = 0; i < items.size(); ++i) {
        if(MetaTreeItem* item = dynamic_cast<MetaTreeItem*>(items.at(i))) {
            QVariant data;
            data.setValue(colDefault);
            item->setData(data, MetaTreeItemRoles::Color);
            item->setData(data, Qt::DecorationRole);
        }
    }
}
