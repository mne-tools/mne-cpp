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
#include "../common/metatreeitem.h"
#include "../../3dhelpers/geometrymultiplier.h"
#include "../../materials/geometrymultipliermaterial.h"

#include <fiff/fiff_constants.h>
#include <fiff/fiff_ch_info.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QMatrix4x4>
#include <Qt3DExtras/QCuboidGeometry>
#include <Qt3DCore/QEntity>
#include <Qt3DExtras/QSphereGeometry>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SensorPositionTreeItem::SensorPositionTreeItem(Qt3DCore::QEntity *p3DEntityParent, int iType, const QString& text)
: Abstract3DTreeItem(p3DEntityParent, iType, text)
{
    initItem();
}


//*************************************************************************************************************

void SensorPositionTreeItem::initItem()
{
    this->setEditable(false);
    this->setCheckable(true);
    this->setCheckState(Qt::Checked);
    this->setToolTip(this->text());
}


//*************************************************************************************************************

void SensorPositionTreeItem::addData(const QList<FIFFLIB::FiffChInfo>& lChInfo, const QString& sDataType)
{
    plotSensors(lChInfo, sDataType);
}


//*************************************************************************************************************

void SensorPositionTreeItem::plotSensors(const QList<FIFFLIB::FiffChInfo>& lChInfo, const QString& sDataType)
{
    //Create digitizers
    Renderable3DEntity* pSensorEntity = new Renderable3DEntity(this);

    GeometryMultiplier *pMesh;

    if(sDataType == "MEG")
    {
        QSharedPointer<Qt3DExtras::QCuboidGeometry> pSensorRect = QSharedPointer<Qt3DExtras::QCuboidGeometry>::create();
        pSensorRect->setXExtent(0.01f);
        pSensorRect->setYExtent(0.01f);
        pSensorRect->setZExtent(0.001f);

        pMesh = new GeometryMultiplier(pSensorRect);

        //Create transform matrix for each cuboid instance
        QVector<QMatrix4x4> vTransforms;
        vTransforms.reserve(lChInfo.size());
        QVector3D tempPos;

        for(int i = 0; i < lChInfo.size(); ++i) {
            QMatrix4x4 tempTransform;

            tempPos.setX(lChInfo[i].chpos.r0(0));
            tempPos.setY(lChInfo[i].chpos.r0(1));
            tempPos.setZ(lChInfo[i].chpos.r0(2));
            //Set position
            tempTransform.translate(tempPos);

            //Set orientation
            for(int j = 0; j < 4; ++j) {
                tempTransform(j, 0) = lChInfo[i].coil_trans.row(j)(0);
                tempTransform(j, 1) = lChInfo[i].coil_trans.row(j)(1);
                tempTransform(j, 2) = lChInfo[i].coil_trans.row(j)(2);
            }

            vTransforms.push_back(tempTransform);
        }

        //Set instance Transform
        pMesh->setTransforms(vTransforms);
    }
    else if (sDataType == "EEG")
    {
        QSharedPointer<Qt3DExtras::QSphereGeometry> pSourceSphere = QSharedPointer<Qt3DExtras::QSphereGeometry>::create();
        pSourceSphere->setRadius(0.001f);

        pMesh = new GeometryMultiplier(pSourceSphere);

        //Create transform matrix for each cuboid instance
        QVector<QMatrix4x4> vTransforms;
        vTransforms.reserve(lChInfo.size());
        QVector3D tempPos;

        for(int i = 0; i < lChInfo.size(); ++i) {
            QMatrix4x4 tempTransform;

            tempPos.setX(lChInfo[i].chpos.r0(0));
            tempPos.setY(lChInfo[i].chpos.r0(1));
            tempPos.setZ(lChInfo[i].chpos.r0(2));
            //Set position
            tempTransform.translate(tempPos);

            vTransforms.push_back(tempTransform);
        }

        //Set instance Transform
        pMesh->setTransforms(vTransforms);
    }

    pSensorEntity->addComponent(pMesh);

    //Add material
    GeometryMultiplierMaterial* pMaterial = new GeometryMultiplierMaterial(true);

    QColor colDefault(100,100,100);
    pMaterial->setAmbient(colDefault);
    pMaterial->setAlpha(1.0f);
    pSensorEntity->addComponent(pMaterial);


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
