//=============================================================================================================
/**
 * @file     sensorpositiontreeitem.cpp
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
 * @brief    SensorPositionTreeItem class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sensorpositiontreeitem.h"
#include "../common/metatreeitem.h"
#include "../../3dhelpers/geometrymultiplier.h"
#include "../../materials/geometrymultipliermaterial.h"

#include <fiff/fiff_constants.h>
#include <fiff/fiff_ch_info.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMatrix4x4>
#include <Qt3DExtras/QCuboidGeometry>
#include <Qt3DCore/QEntity>
#include <Qt3DExtras/QSphereGeometry>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SensorPositionTreeItem::SensorPositionTreeItem(Qt3DCore::QEntity *p3DEntityParent, int iType, const QString& text)
: Abstract3DTreeItem(p3DEntityParent, iType, text)
{
    initItem();
}

//=============================================================================================================

void SensorPositionTreeItem::initItem()
{
    this->setEditable(false);
    this->setCheckable(true);
    this->setCheckState(Qt::Checked);
    this->setToolTip(this->text());
}

//=============================================================================================================

void SensorPositionTreeItem::addData(const QList<FIFFLIB::FiffChInfo>& lChInfo,
                                     const QString& sDataType,
                                     const QStringList& bads)
{
    if(sDataType == "MEG") {
        plotMEGSensors(lChInfo, bads);
    } else if (sDataType == "EEG") {
        plotEEGSensors(lChInfo, bads);
    }
}

//=============================================================================================================

void SensorPositionTreeItem::plotMEGSensors(const QList<FIFFLIB::FiffChInfo>& lChInfo,
                                            const QStringList& bads)
{
    if(lChInfo.isEmpty()) {
        qDebug() << "SensorPositionTreeItem::plotMEGSensors - Channel data is empty. Returning.";
        return;
    }

    if(!m_pMEGSensorEntity) {
        m_pMEGSensorEntity = new QEntity(this);
    }

    //create geometry
    if(!m_pMEGSensors) {
        if(!m_pMEGSensorGeometry) {
            m_pMEGSensorGeometry = QSharedPointer<Qt3DExtras::QCuboidGeometry>::create();
            m_pMEGSensorGeometry->setXExtent(0.01f);
            m_pMEGSensorGeometry->setYExtent(0.01f);
            m_pMEGSensorGeometry->setZExtent(0.001f);
        }

        m_pMEGSensors = new GeometryMultiplier(m_pMEGSensorGeometry);

        m_pMEGSensorEntity->addComponent(m_pMEGSensors);

        //Add material
        GeometryMultiplierMaterial* pMaterial = new GeometryMultiplierMaterial(false);
        pMaterial->setAmbient(QColor(100,100,100));
        pMaterial->setAlpha(0.75);
        m_pMEGSensorEntity->addComponent(pMaterial);
    }

    //Create transform matrix for each cuboid instance
    QVector<QMatrix4x4> vTransforms;
    vTransforms.reserve(lChInfo.size());
    QVector<QColor> vColorsNodes;
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

        if(!vTransforms.contains(tempTransform)) {
            vTransforms.push_back(tempTransform);
        }

        if(bads.contains(lChInfo.at(i).ch_name)) {
            vColorsNodes.push_back(QColor(255,0,0));
        } else {
            vColorsNodes.push_back(QColor(100,100,100));
        }
    }

    //Set instance Transform
    m_pMEGSensors->setTransforms(vTransforms);
    m_pMEGSensors->setColors(vColorsNodes);

    //Update colors in color item
    QList<QStandardItem*> items = this->findChildren(MetaTreeItemTypes::Color);

    for(int i = 0; i < items.size(); ++i) {
        if(MetaTreeItem* item = dynamic_cast<MetaTreeItem*>(items.at(i))) {
            QVariant data;
            data.setValue(QColor(100,100,100));
            item->setData(data, MetaTreeItemRoles::Color);
            item->setData(data, Qt::DecorationRole);
        }
    }
}

//=============================================================================================================

void SensorPositionTreeItem::plotEEGSensors(const QList<FIFFLIB::FiffChInfo>& lChInfo,
                                            const QStringList& bads)
{
    if(lChInfo.isEmpty()) {
        qDebug() << "SensorPositionTreeItem::plotEEGSensors - Channel data is empty. Returning.";
        return;
    }

    if(!m_pEEGSensorEntity) {
        m_pEEGSensorEntity = new QEntity(this);
    }

    //create geometry
    if(!m_pEEGSensors) {
        if(!m_pEEGSensorGeometry) {
            m_pEEGSensorGeometry = QSharedPointer<Qt3DExtras::QSphereGeometry>::create();
            m_pEEGSensorGeometry->setRadius(0.001f);
        }

        m_pEEGSensors = new GeometryMultiplier(m_pEEGSensorGeometry);

        m_pEEGSensorEntity->addComponent(m_pEEGSensors);

        //Add material
        GeometryMultiplierMaterial* pMaterial = new GeometryMultiplierMaterial(false);
        pMaterial->setAmbient(QColor(100,100,100));
        pMaterial->setAlpha(0.75);
        m_pEEGSensorEntity->addComponent(pMaterial);
    }

    //Create transform matrix for each cuboid instance
    QVector<QMatrix4x4> vTransforms;
    vTransforms.reserve(lChInfo.size());
    QVector<QColor> vColorsNodes;
    QVector3D tempPos;

    for(int i = 0; i < lChInfo.size(); ++i) {
        QMatrix4x4 tempTransform;

        tempPos.setX(lChInfo[i].chpos.r0(0));
        tempPos.setY(lChInfo[i].chpos.r0(1));
        tempPos.setZ(lChInfo[i].chpos.r0(2));
        //Set position
        tempTransform.translate(tempPos);

        vTransforms.push_back(tempTransform);

        if(bads.contains(lChInfo.at(i).ch_name)) {
            vColorsNodes.push_back(Qt::red);
        } else {
            vColorsNodes.push_back(Qt::gray);
        }
    }

    //Set instance Transform
    m_pEEGSensors->setTransforms(vTransforms);
    m_pEEGSensors->setColors(vColorsNodes);

    //Update colors in color item
    QList<QStandardItem*> items = this->findChildren(MetaTreeItemTypes::Color);

    for(int i = 0; i < items.size(); ++i) {
        if(MetaTreeItem* item = dynamic_cast<MetaTreeItem*>(items.at(i))) {
            QVariant data;
            data.setValue(QColor(100,100,100));
            item->setData(data, MetaTreeItemRoles::Color);
            item->setData(data, Qt::DecorationRole);
        }
    }
}
