//=============================================================================================================
/**
 * @file     digitizertreeitem.cpp
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lars Debor, Lorenz Esch. All rights reserved.
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "digitizertreeitem.h"
#include "../common/metatreeitem.h"
#include "../../3dhelpers/geometrymultiplier.h"
#include "../../materials/geometrymultipliermaterial.h"

#include <fiff/fiff_constants.h>
#include <fiff/fiff_dig_point.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <Qt3DExtras/QSphereGeometry>
#include <QMatrix4x4>
#include <Qt3DCore/QTransform>

//=============================================================================================================
// EIGEN INCLUDES
//============================================================================= ================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DigitizerTreeItem::DigitizerTreeItem(Qt3DCore::QEntity *p3DEntityParent, int iType, const QString& text)
: Abstract3DTreeItem(p3DEntityParent, iType, text)
{
    initItem();
}

//=============================================================================================================

void DigitizerTreeItem::initItem()
{
    this->setEditable(false);
    this->setCheckable(true);
    this->setCheckState(Qt::Checked);
    this->setToolTip(this->text());
}

//=============================================================================================================

void DigitizerTreeItem::addData(const QList<FIFFLIB::FiffDigPoint>& tDigitizer, const float tSphereRadius, const QColor &tSphereColor)
{
    //create instanced renderer if there is none
    if(!m_pSphereMesh && !tDigitizer.isEmpty())
    {
        QSharedPointer<Qt3DExtras::QSphereGeometry> pSourceSphereGeometry = QSharedPointer<Qt3DExtras::QSphereGeometry>::create();
        pSourceSphereGeometry->setRadius(tSphereRadius);

        m_pSphereMesh = new GeometryMultiplier(pSourceSphereGeometry);

        //Set sphere positions
        this->addComponent(m_pSphereMesh);

        //Add material
        GeometryMultiplierMaterial* pMaterial = new GeometryMultiplierMaterial;
        pMaterial->setAmbient(tSphereColor);
        this->addComponent(pMaterial);
    }

    //Set transforms
    if(!tDigitizer.isEmpty())
    {
        QVector<QColor> vColorsEdges;

        QVector<QMatrix4x4> vTransforms;
        vTransforms.reserve(tDigitizer.size());

        QVector3D tempPos;

        for(int i = 0; i < tDigitizer.size(); ++i) {
            QMatrix4x4 tempTransform;

            tempPos.setX(tDigitizer[i].r[0]);
            tempPos.setY(tDigitizer[i].r[1]);
            tempPos.setZ(tDigitizer[i].r[2]);

            tempTransform.translate(tempPos);
            vTransforms.push_back(tempTransform);

            vColorsEdges.push_back(tSphereColor);
        }

        //Set instance Transform
        m_pSphereMesh->setTransforms(vTransforms);
        m_pSphereMesh->setColors(vColorsEdges);
    }

    //Update alpha
    float alpha = 1.0;

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
            data.setValue(tSphereColor);
            item->setData(data, MetaTreeItemRoles::Color);
            item->setData(data, Qt::DecorationRole);
        }
    }
}
