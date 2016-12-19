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
* @brief    ECDDataTreeItem class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ecddatatreeitem.h"
#include "../common/metatreeitem.h"
#include "../common/renderable3Dentity.h"


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

ECDDataTreeItem::ECDDataTreeItem(int iType, const QString &text)
: AbstractTreeItem(iType, text)
, m_bIsInit(false)
{
    this->setEditable(false);
    this->setToolTip("Dipole fit source localization data");
}


//*************************************************************************************************************

ECDDataTreeItem::~ECDDataTreeItem()
{
    //Schedule deletion/Decouple of all entities so that the SceneGraph is NOT plotting them anymore.
    //Cannot delete m_pParentEntity since we do not know who else holds it, that is why we use a QPointer for m_pParentEntity.
    for(int i = 0; i < m_lDipoles.size(); ++i) {
        m_lDipoles.at(i)->deleteLater();
    }

    if(!m_pRenderable3DEntity.isNull()) {
        m_pRenderable3DEntity->deleteLater();
    }
}


//*************************************************************************************************************

QVariant ECDDataTreeItem::data(int role) const
{
    return AbstractTreeItem::data(role);
}


//*************************************************************************************************************

void ECDDataTreeItem::setData(const QVariant& value, int role)
{
    AbstractTreeItem::setData(value, role);
}


//*************************************************************************************************************

bool ECDDataTreeItem::init(Qt3DCore::QEntity* parent)
{      
    //Create renderable 3D entity
    m_pParentEntity = parent;
    m_pRenderable3DEntity = new Renderable3DEntity(parent);

    //Add meta information as item children
    QList<QStandardItem*> list;
    QVariant data;

    m_bIsInit = true;

    return true;
}


//*************************************************************************************************************

bool ECDDataTreeItem::addData(ECDSet::SPtr pECDSet)
{
    if(!m_bIsInit) {
        qDebug() << "NetworkTreeItem::updateData - NetworkTreeItem has not been initialized yet!";
        return false;
    }

    //Add data which is held by this NetworkTreeItem
    QVariant data;

    data.setValue(pECDSet);
    this->setData(data, Data3DTreeModelItemRoles::ECDSetData);

    //Plot dipole moment
    plotDipoles(pECDSet);

    return true;
}


//*************************************************************************************************************

void ECDDataTreeItem::onCheckStateChanged(const Qt::CheckState& checkState)
{
    this->setVisible(checkState == Qt::Unchecked ? false : true);
}


//*************************************************************************************************************

void ECDDataTreeItem::setVisible(bool state)
{
    for(int i = 0; i < m_lDipoles.size(); ++i) {
        m_lDipoles.at(i)->setParent(state ? m_pRenderable3DEntity : Q_NULLPTR);
    }

    m_pRenderable3DEntity->setParent(state ? m_pParentEntity : Q_NULLPTR);
}


//*************************************************************************************************************

void ECDDataTreeItem::plotDipoles(QSharedPointer<ECDSet> pECDSet)
{
    //Plot dipoles
    QVector3D pos,to,from;

    for(int i = 0; i < pECDSet->size(); ++i) {
        pos.setX((*pECDSet)[i].rd(0));
        pos.setY((*pECDSet)[i].rd(1));
        pos.setZ((*pECDSet)[i].rd(2));

        to.setX((*pECDSet)[i].Q(0));
        to.setY((*pECDSet)[i].Q(1));
        to.setZ((*pECDSet)[i].Q(2));

//        from = QVector3D(0.0, 1.0, 0.0) - pos;
//        from = from.normalized();

//        qDebug()<<"ECDDataTreeItem::plotDipoles - from" << from;
//        qDebug()<<"ECDDataTreeItem::plotDipoles - to" << to;

//        pos *= 1000;
//        pos = pos.normalized();

//        to *= 1000;
//        to = to.normalized();

//        QQuaternion final = QQuaternion::rotationTo(from, to);
//        pos = final.rotatedVector(pos)/1000;

        Renderable3DEntity* dipoleEntity = new Renderable3DEntity(m_pRenderable3DEntity);

        Qt3DExtras::QConeMesh* dipoleCone = new Qt3DExtras::QConeMesh();
        dipoleCone->setBottomRadius(0.001f);
        dipoleCone->setLength(0.003f);

        dipoleEntity->addComponent(dipoleCone);

        //Set dipole position and orientation
        Qt3DCore::QTransform* transform = new Qt3DCore::QTransform();
        QMatrix4x4 m;
        m.translate(pos);
        transform->setMatrix(m);
        dipoleEntity->addComponent(transform);

        Qt3DExtras::QPhongMaterial* material = new Qt3DExtras::QPhongMaterial();
        material->setAmbient(QColor(rand()%255, rand()%255, rand()%255));
        dipoleEntity->addComponent(material);

        m_lDipoles.append(dipoleEntity);
    }
}


