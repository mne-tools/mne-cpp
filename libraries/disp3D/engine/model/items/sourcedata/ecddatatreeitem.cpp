//=============================================================================================================
/**
 * @file     ecddatatreeitem.cpp
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     November, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Lars Debor, Gabriel B Motta, Lorenz Esch. All rights reserved.
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
 * @brief    EcdDataTreeItem class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ecddatatreeitem.h"
#include "../common/metatreeitem.h"

#include <inverse/dipoleFit/ecd_set.h>
#include "../../3dhelpers/geometrymultiplier.h"
#include "../../materials/geometrymultipliermaterial.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>
#include <QVariant>
#include <QStringList>
#include <QColor>
#include <QVector3D>
#include <QStandardItem>
#include <QStandardItemModel>
#include <Qt3DRender/qshaderprogram.h>
#include <QQuaternion>
#include <Qt3DExtras/QConeGeometry>
#include <QMatrix4x4>
#include <QRandomGenerator>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace INVERSELIB;
using namespace DISP3DLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EcdDataTreeItem::EcdDataTreeItem(Qt3DCore::QEntity *p3DEntityParent, int iType, const QString &text)
: Abstract3DTreeItem(p3DEntityParent, iType, text)
{
    initItem();
}

//=============================================================================================================

void EcdDataTreeItem::initItem()
{
    this->setEditable(false);
    this->setCheckable(true);
    this->setCheckState(Qt::Checked);
    this->setToolTip("Dipole fit data");
}

//=============================================================================================================

void EcdDataTreeItem::addData(const ECDSet& pECDSet)
{
    if(!m_pItemNumDipoles){
        //Add further infos as children
        QList<QStandardItem*> list;
        m_pItemNumDipoles = new MetaTreeItem(MetaTreeItemTypes::NumberDipoles, QString::number(pECDSet.size()));
        m_pItemNumDipoles->setEditable(false);
        list.clear();
        list << m_pItemNumDipoles;
        list << new QStandardItem(m_pItemNumDipoles->toolTip());
        this->appendRow(list);
    } else {
        m_pItemNumDipoles->setText(QString::number(pECDSet.size()));
    }
    //Plot dipole moment
    plotDipoles(pECDSet);
}

//=============================================================================================================

void EcdDataTreeItem::plotDipoles(const ECDSet& tECDSet)
{
    //Plot dipoles

    QVector3D pos, to, from;

    //The Qt3D default cone orientation and the top of the cone lies in line with the positive y-axis.
    from = QVector3D(0.0, 1.0, 0.0);
    double norm;

    QVector<QColor> vColors;
    vColors.reserve(tECDSet.size());
    QVector<QMatrix4x4> vTransforms;
    vTransforms.reserve(tECDSet.size());

    for(int i = 0; i < tECDSet.size(); ++i) {
        pos.setX(tECDSet[i].rd(0));
        pos.setY(tECDSet[i].rd(1));
        pos.setZ(tECDSet[i].rd(2));

        norm = sqrt(pow(tECDSet[i].Q(0),2)+pow(tECDSet[i].Q(1),2)+pow(tECDSet[i].Q(2),2));

        to.setX(tECDSet[i].Q(0)/norm);
        to.setY(tECDSet[i].Q(1)/norm);
        to.setZ(tECDSet[i].Q(2)/norm);

//        qDebug()<<"EcdDataTreeItem::plotDipoles - from" << from;
//        qDebug()<<"EcdDataTreeItem::plotDipoles - to" << to;

        QQuaternion final = QQuaternion::rotationTo(from, to);

        //Set dipole position and orientation
        QMatrix4x4 m;
        m.translate(pos);
        m.rotate(final);
        vTransforms.push_back(m);

        //add random color;
        vColors.push_back(QColor(QRandomGenerator::global()->bounded(0 , 255),
                                 QRandomGenerator::global()->bounded(0 , 255),
                                 QRandomGenerator::global()->bounded(0 , 255)));
    }

    //create geometry
    if(!m_pDipolMesh){
        QSharedPointer<Qt3DExtras::QConeGeometry> pDipolGeometry = QSharedPointer<Qt3DExtras::QConeGeometry>::create();
        pDipolGeometry->setBottomRadius(0.001f);
        pDipolGeometry->setLength(0.003f);
        //create instanced renderer
        m_pDipolMesh = new GeometryMultiplier(pDipolGeometry);

        //Set instance Transform
        m_pDipolMesh->setTransforms(vTransforms);
        //Set instance colors
        m_pDipolMesh->setColors(vColors);

        this->addComponent(m_pDipolMesh);

        //Add material
        GeometryMultiplierMaterial* pMaterial = new GeometryMultiplierMaterial;
        pMaterial->setAmbient(QColor(0,0,0));
        pMaterial->setAlpha(1.0f);
        this->addComponent(pMaterial);
    } else {
        //Set instance Transform
        m_pDipolMesh->setTransforms(vTransforms);
        //Set instance colors
        m_pDipolMesh->setColors(vColors);
    }
}

