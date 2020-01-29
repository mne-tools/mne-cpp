//=============================================================================================================
/**
* @file     centralview.cpp
* @author   Simon Heinke <simon.heinke@tu-ilmenau.de>;
*           Lars Debor <lars.debor@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2018
*
* @section  LICENSE
*
* Copyright (C) 2018, Simon Heinke, Lars Debor and Matti Hamalainen. All rights reserved.
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
* @brief    CentralView class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "centralview.h"
#include "../../../libraries/disp3D/engine/view/orbitalcameracontroller.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <Qt3DRender>
#include <Qt3DExtras>
#include <QPickingSettings>
#include <QCamera>
#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MAINVIEWEREXTENSION;
using namespace Qt3DRender;
using namespace Qt3DCore;
using namespace Qt3DExtras;
using namespace DISP3DLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

CentralView::CentralView()
    : Qt3DWindow(),
      m_pRootEntity(new QEntity),
      m_pCamera(this->camera()),
      m_vEntities(),
      m_vAntiCrashNodes()
{
    init();
}

//*************************************************************************************************************

void CentralView::init()
{
    // initialize 3D Window
    QPickingSettings *pPickSettings = renderSettings()->pickingSettings();
    pPickSettings->setFaceOrientationPickingMode(QPickingSettings::FrontAndBackFace);
    pPickSettings->setPickMethod(QPickingSettings::TrianglePicking);
    pPickSettings->setPickResultMode(QPickingSettings::NearestPick);

    m_pCamera->lens()->setPerspectiveProjection(45.0f, 16.0f/9.0f, 0.0001f, 100000.0f);
    m_pCamera->setPosition(QVector3D(0.0f, -0.4f, -0.25f));
    m_pCamera->setViewCenter(QVector3D(0.0f, 0.0f, 0.0f));
    m_pCamera->setUpVector(QVector3D(0.0f, 1.0f, 0.0f));
    m_pCamera->tiltAboutViewCenter(180);

    OrbitalCameraController *pCamController = new OrbitalCameraController(m_pRootEntity);
    pCamController->setCamera(m_pCamera);

    // we introduced the convention that every entity below root should be named
    pCamController->setObjectName("MainViewer/CameraController");
    m_vEntities.push_back(QSharedPointer<QEntity>(pCamController));

    setRootEntity(m_pRootEntity);
}


//*************************************************************************************************************

void CentralView::addEntity(QSharedPointer<QEntity> pEntity)
{
    // check if passed entity tree fulfills specified requirements
    if (pEntity->objectName().size() == 0) {
        qDebug() << "[CentralView::addEntity] Received Entity without a name !";
        return;
    }

    for (const QSharedPointer<QEntity> pStoredEntity : m_vEntities) {
        if (pStoredEntity == pEntity) {
            qDebug() << "[CentralView::addEntity] The Entity named " << pEntity->objectName() << " is already in the record !";
            return;
        } else if (pStoredEntity->objectName().compare(pEntity->objectName()) == 0) {
            qDebug() << "[CentralView::addEntity] There is a different Entity named " << pEntity->objectName();
            return;
        }
    }

    // remember shared pointer
    m_vEntities.push_back(pEntity);

    // simply insert below root
    pEntity->setParent(m_pRootEntity);

    // use this as an opportunity to check for unused anti crash nodes
    checkForUnusedAntiCrashNodes();
}


//*************************************************************************************************************

void CentralView::removeEntity(QSharedPointer<QEntity> pEntity)
{
    // consistency check:
    QEntity* pSupposedChild = pEntity.data();
    // see if we can find the same QEntity by name
    const QString& sIdentifier = pEntity->objectName();
    if (sIdentifier.length() == 0) {
        qDebug() << "[CentralView::removeEntity] Passed entity does not have a name, returning...";
        return;
    }
    // only search for direct children, since entities are always added below root
    QEntity* pActualChild = m_pRootEntity->findChild<QEntity* >(sIdentifier, Qt::FindDirectChildrenOnly);
    if (pActualChild) {
        if (pActualChild != pSupposedChild) {
            qDebug() << "[CentralView::removeEntity] CRITICAL: Inconsistency in identifiers ! Removing both sides !";
            // best thing we can do is remove both QEntities from this View:
            pSupposedChild->setParent(createNewAntiCrashNode());
            pActualChild->setParent(createNewAntiCrashNode());
            return;
        }
    } else {
        // this is not as bad as the above inconsistency, might be caused by calling removeEntity prior to addEntity
        // just give out a warning:
        qDebug() << "[CentralView::removeEntity] Could not find child named " << sIdentifier << " ! Did you call addEntity ?";
        return;
    }

    // consistency check went through, this is the usual case:
    m_vEntities.remove(m_vEntities.indexOf(pEntity));
    // create a new anti crash node and make it the new parent of the remove-candidate
    pActualChild->setParent(createNewAntiCrashNode());

    // use this as an opportunity to check for unused anti crash nodes
    checkForUnusedAntiCrashNodes();
}


//*************************************************************************************************************

void CentralView::shutdown()
{
    // dissassemble the used entity tree
    QNodeVector vChildren = m_pRootEntity->childNodes();
    for (QNode* pNode : vChildren) {
        QEntity* pTemp = (QEntity*) pNode;
        if (pTemp) {
            // avoid double frees on program shutdown, caused by shared ownership
            pTemp->setParent((QEntity*) nullptr);
        }
    }

    // take care of dangling anti crash nodes
    for (int i = 0; i < m_vAntiCrashNodes.size(); ++i) {
        int iNumChildren = m_vAntiCrashNodes.at(i)->childNodes().count();
        if (iNumChildren == 0) {
            // not used anymore, simply wait for vector destructor
        }
        else if (iNumChildren == 1) {
            // still used, need to seperate the child from the anti crash nodes in order to avoid double frees
            m_vAntiCrashNodes.at(i)->childNodes().at(0)->setParent((QEntity*) nullptr);
        }
        else {
            qDebug() << "[CentralView::shutdown] CRITICAL: found anti crash node with more than one child !";
            // best thing we can do is to seperate the parent anti crash node from every child
            for (QNode* pNode : m_vAntiCrashNodes.at(i)->childNodes())
            {
                pNode->setParent((QEntity*) nullptr);
            }
        }
    }
}

//*************************************************************************************************************

QEntity* CentralView::createNewAntiCrashNode()
{
    QSharedPointer<QEntity> pAntiCrashNode = QSharedPointer<QEntity>::create();
    m_vAntiCrashNodes.push_back(pAntiCrashNode);
    return pAntiCrashNode.data();
}


//*************************************************************************************************************

void CentralView::checkForUnusedAntiCrashNodes()
{
    for (int i = 0; i < m_vAntiCrashNodes.size(); ++i) {
        if (m_vAntiCrashNodes.at(i)->childNodes().count() == 0) {
            // no longer needed, remove and wait for shared pointer destruction
            m_vAntiCrashNodes.remove(i);
            --i;
        }
    }
}
