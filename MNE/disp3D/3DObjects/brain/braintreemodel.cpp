//=============================================================================================================
/**
* @file     braintreemodel.cpp
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
* @brief    BrainTreeModel class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "braintreemodel.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace MNELIB;
using namespace FSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainTreeModel::BrainTreeModel(QObject* parent)
: QStandardItemModel(parent)
{
    m_pRootItem = this->invisibleRootItem();
    m_pRootItem->setText("Loaded 3D Data");
}


//*************************************************************************************************************

BrainTreeModel::~BrainTreeModel()
{
    delete m_pRootItem;
}


//*************************************************************************************************************

QVariant BrainTreeModel::data(const QModelIndex& index, int role) const
{
    return QStandardItemModel::data(index, role);
}


//*************************************************************************************************************

bool BrainTreeModel::addData(const QString& text, const SurfaceSet& tSurfaceSet, const AnnotationSet& tAnnotationSet, Qt3DCore::QEntity* p3DEntityParent)
{
    //Find already existing surface items and add the new data to the first search result
    QList<QStandardItem*> itemList = this->findItems(text);
    bool state = false;

    if(!itemList.isEmpty() && (itemList.at(0)->type() == BrainTreeModelItemTypes::SurfaceSetItem)) {
        BrainSurfaceSetTreeItem* pSurfaceSetItem = dynamic_cast<BrainSurfaceSetTreeItem*>(itemList.at(0));
        state = pSurfaceSetItem->addData(tSurfaceSet, tAnnotationSet, p3DEntityParent);
    } else {
        BrainSurfaceSetTreeItem* pSurfaceSetItem = new BrainSurfaceSetTreeItem(BrainTreeModelItemTypes::SurfaceSetItem, text);
        m_pRootItem->appendRow(pSurfaceSetItem);
        state = pSurfaceSetItem->addData(tSurfaceSet, tAnnotationSet, p3DEntityParent);
    }

    return state;
}


//*************************************************************************************************************

bool BrainTreeModel::addData(const QString& text, const Surface& tSurface, const Annotation& tAnnotation, Qt3DCore::QEntity* p3DEntityParent)
{
    //Find already existing surface items and add the new data to the first search result
    QList<QStandardItem*> itemList = this->findItems(text);
    bool state = false;

    if(!itemList.isEmpty() && (itemList.at(0)->type() == BrainTreeModelItemTypes::SurfaceSetItem)) {
        BrainSurfaceSetTreeItem* pSurfaceSetItem = dynamic_cast<BrainSurfaceSetTreeItem*>(itemList.at(0));
        state = pSurfaceSetItem->addData(tSurface, tAnnotation, p3DEntityParent);
    } else {
        BrainSurfaceSetTreeItem* pSurfaceSetItem = new BrainSurfaceSetTreeItem(BrainTreeModelItemTypes::SurfaceSetItem, text);
        m_pRootItem->appendRow(pSurfaceSetItem);
        state = pSurfaceSetItem->addData(tSurface, tAnnotation, p3DEntityParent);
    }

    return state;
}


//*************************************************************************************************************

bool BrainTreeModel::addData(const QString& text, const MNESourceSpace& tSourceSpace, Qt3DCore::QEntity* p3DEntityParent)
{
    //Find already existing surface items and add the new data to the first search result
    QList<QStandardItem*> itemList = this->findItems(text);
    bool state = false;

    if(!itemList.isEmpty() && (itemList.at(0)->type() == BrainTreeModelItemTypes::SurfaceSetItem)) {
        BrainSurfaceSetTreeItem* pSurfaceSetItem = dynamic_cast<BrainSurfaceSetTreeItem*>(itemList.at(0));
        state = pSurfaceSetItem->addData(tSourceSpace, p3DEntityParent);
    } else {
        BrainSurfaceSetTreeItem* pSurfaceSetItem = new BrainSurfaceSetTreeItem(BrainTreeModelItemTypes::SurfaceSetItem, text);
        m_pRootItem->appendRow(pSurfaceSetItem);
        state = pSurfaceSetItem->addData(tSourceSpace, p3DEntityParent);
    }

    return state;
}


//*************************************************************************************************************

QList<BrainRTSourceLocDataTreeItem*> BrainTreeModel::addData(const QString& text, const MNESourceEstimate& tSourceEstimate, const MNEForwardSolution& tForwardSolution)
{
    QList<BrainRTSourceLocDataTreeItem*> returnList;
    QList<QStandardItem*> itemList = this->findItems(text);

    //Find the all the hemispheres of the set "text" and add the source estimates as items
    if(!itemList.isEmpty()) {
        for(int i = 0; i<itemList.size(); i++) {
            for(int j = 0; j<itemList.at(i)->rowCount(); j++) {
                if(itemList.at(i)->child(j,0)->type() == BrainTreeModelItemTypes::HemisphereItem) {
                    BrainHemisphereTreeItem* pHemiItem = dynamic_cast<BrainHemisphereTreeItem*>(itemList.at(i)->child(j,0));
                    returnList.append(pHemiItem->addData(tSourceEstimate, tForwardSolution));
                }
            }
        }
    }

    return returnList;
}


