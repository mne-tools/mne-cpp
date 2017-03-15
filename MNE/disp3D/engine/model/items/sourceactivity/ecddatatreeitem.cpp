//=============================================================================================================
/**
* @file     ecddatatreeitem.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     November, 2015
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
* @brief    EcdDataTreeItem class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ecddatatreeitem.h"
#include "../common/metatreeitem.h"
#include "../../3dhelpers/renderable3Dentity.h"

#include <inverse/dipoleFit/ecd_set.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QList>
#include <QVariant>
#include <QStringList>
#include <QColor>
#include <QVector3D>
#include <QStandardItem>
#include <QStandardItemModel>
#include <Qt3DRender/qshaderprogram.h>
#include <Qt3DExtras/QSphereMesh>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DCore/QTransform>
#include <QQuaternion>
#include <Qt3DExtras/QConeMesh>


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
using namespace INVERSELIB;
using namespace DISP3DLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EcdDataTreeItem::EcdDataTreeItem(int iType, const QString &text)
: AbstractTreeItem(iType, text)
, m_bIsDataInit(false)
, m_pRenderable3DEntity(new Renderable3DEntity())
{
    initItem();
}


//*************************************************************************************************************

EcdDataTreeItem::~EcdDataTreeItem()
{
    //Schedule deletion/Decouple of all entities so that the SceneGraph is NOT plotting them anymore.
    for(int i = 0; i < m_lDipoles.size(); ++i) {
        m_lDipoles.at(i)->deleteLater();
    }

    if(m_pRenderable3DEntity) {
        m_pRenderable3DEntity->deleteLater();
    }
}


//*************************************************************************************************************

void EcdDataTreeItem::initItem()
{
    this->setEditable(false);
    this->setCheckable(true);
    this->setCheckState(Qt::Checked);
    this->setToolTip("Dipole fit data");
}


//*************************************************************************************************************

QVariant EcdDataTreeItem::data(int role) const
{
    return AbstractTreeItem::data(role);
}


//*************************************************************************************************************

void EcdDataTreeItem::setData(const QVariant& value, int role)
{
    AbstractTreeItem::setData(value, role);
}


//*************************************************************************************************************

void EcdDataTreeItem::initData(Qt3DCore::QEntity* parent)
{      
    //Create renderable 3D entity
    m_pRenderable3DEntity->setParent(parent);

    m_bIsDataInit = true;
}


//*************************************************************************************************************

void EcdDataTreeItem::addData(const ECDSet& pECDSet)
{
    if(!m_bIsDataInit) {
        qDebug() << "EcdDataTreeItem::addData - EcdDataTreeItem data has not been initialized yet!";
        return;
    }

    //Add further infos as children
    QList<QStandardItem*> list;
    MetaTreeItem *pItemNumDipoles = new MetaTreeItem(MetaTreeItemTypes::NumberDipoles, QString::number(pECDSet.size()));
    pItemNumDipoles->setEditable(false);
    list.clear();
    list << pItemNumDipoles;
    list << new QStandardItem(pItemNumDipoles->toolTip());
    this->appendRow(list);

    //Add data which is held by this NetworkTreeItem
    QVariant data;

    data.setValue(pECDSet);
    this->setData(data, Data3DTreeModelItemRoles::ECDSetData);

    //Plot dipole moment
    plotDipoles(pECDSet);
}


//*************************************************************************************************************

void EcdDataTreeItem::onCheckStateChanged(const Qt::CheckState& checkState)
{
    this->setVisible(checkState == Qt::Unchecked ? false : true);
}


//*************************************************************************************************************

void EcdDataTreeItem::setVisible(bool state)
{
    for(int i = 0; i < m_lDipoles.size(); ++i) {
        m_lDipoles.at(i)->setEnabled(state);
    }

    m_pRenderable3DEntity->setEnabled(state);
}


//*************************************************************************************************************

void EcdDataTreeItem::plotDipoles(const ECDSet& tECDSet)
{
    //Plot dipoles
    QVector3D pos, to, from;

    //The Qt3D default cone orientation and the top of the cone lies in line with the positive y-axis.
    from = QVector3D(0.0, 1.0, 0.0);
    double norm;

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

        Renderable3DEntity* dipoleEntity = new Renderable3DEntity(m_pRenderable3DEntity);

        Qt3DExtras::QConeMesh* dipoleCone = new Qt3DExtras::QConeMesh();
        dipoleCone->setBottomRadius(0.001f);

//        //Calculate cone length based on norm 0.1mm -> 1nAm
//        double cm = 0.001;
//        double scale = (pow(10,-9))/(cm*0.01);
//        dipoleCone->setLength(norm/scale);
////        qDebug()<<"setLength"<<norm/scale;

        dipoleCone->setLength(0.003f);

        dipoleEntity->addComponent(dipoleCone);

        //Set dipole position and orientation
        Qt3DCore::QTransform* transform = new Qt3DCore::QTransform();
        QMatrix4x4 m;
        m.translate(pos);
        m.rotate(final);
        transform->setMatrix(m);
        dipoleEntity->addComponent(transform);

        Qt3DExtras::QPhongMaterial* material = new Qt3DExtras::QPhongMaterial();
        material->setAmbient(QColor(rand()%255, rand()%255, rand()%255));
        dipoleEntity->addComponent(material);

        m_lDipoles.append(dipoleEntity);
    }
}


