//=============================================================================================================
/**
* @file     brainsurfacesettreeitem.cpp
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
* @brief    BrainSurfaceSetTreeItem class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainsurfacesettreeitem.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;
using namespace MNELIB;
using namespace DISP3DLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainSurfaceSetTreeItem::BrainSurfaceSetTreeItem(int iType, const QString& text)
: AbstractTreeItem(iType, text)
{
    this->setEditable(false);
    this->setCheckable(true);
    this->setCheckState(Qt::Checked);
    this->setToolTip("Brain surface set");
}


//*************************************************************************************************************

BrainSurfaceSetTreeItem::~BrainSurfaceSetTreeItem()
{
}


//*************************************************************************************************************

QVariant BrainSurfaceSetTreeItem::data(int role) const
{
    switch(role) {
        case BrainSurfaceSetTreeItemRoles::SurfaceSetName:
            return QVariant();
    }

    return AbstractTreeItem::data(role);
}


//*************************************************************************************************************

void  BrainSurfaceSetTreeItem::setData(const QVariant& value, int role)
{
    AbstractTreeItem::setData(value, role);
}


//*************************************************************************************************************

bool BrainSurfaceSetTreeItem::addData(const SurfaceSet& tSurfaceSet, const AnnotationSet& tAnnotationSet, Qt3DCore::QEntity* p3DEntityParent)
{
    //Generate child items based on surface set input parameters
    bool state = false;

    QList<QStandardItem*> itemList = this->findChildren(BrainTreeModelItemTypes::HemisphereItem);

    //If there are more hemispheres in tSourceSpace than in the tree model
    bool hemiItemFound = false;

    //Search for already created hemi items and add source space data respectivley
    for(int i = 0; i < tSurfaceSet.size(); i++) {
        for(int j = 0; j<itemList.size(); j++) {
            BrainHemisphereTreeItem* pHemiItem = dynamic_cast<BrainHemisphereTreeItem*>(itemList.at(j));

            if(pHemiItem->data(BrainHemisphereTreeItemRoles::SurfaceHemi).toInt() == tSurfaceSet[i].hemi()) {
                hemiItemFound = true;

                if(i < tAnnotationSet.size()) {
                    if(tAnnotationSet[i].hemi() == tSurfaceSet[i].hemi()) {
                        state = pHemiItem->addData(tSurfaceSet[i], tAnnotationSet[i], p3DEntityParent);
                    } else {
                        state = pHemiItem->addData(tSurfaceSet[i], Annotation(), p3DEntityParent);
                    }
                } else {
                    state = pHemiItem->addData(tSurfaceSet[i], Annotation(), p3DEntityParent);
                }
            }
        }

        if(!hemiItemFound) {
            //Item does not exist yet, create it here.
            BrainHemisphereTreeItem* pHemiItem = new BrainHemisphereTreeItem(BrainTreeModelItemTypes::HemisphereItem);

            QList<QStandardItem*> list;
            list<<pHemiItem;
            list<<new QStandardItem(pHemiItem->toolTip());
            this->appendRow(list);

            if(i < tAnnotationSet.size()) {
                if(tAnnotationSet[i].hemi() == tSurfaceSet[i].hemi()) {
                    state = pHemiItem->addData(tSurfaceSet[i], tAnnotationSet[i], p3DEntityParent);
                } else {
                    state = pHemiItem->addData(tSurfaceSet[i], Annotation(), p3DEntityParent);
                }
            } else {
                state = pHemiItem->addData(tSurfaceSet[i], Annotation(), p3DEntityParent);
            }
        }

        hemiItemFound = false;
    }

    return state;
}


//*************************************************************************************************************

bool BrainSurfaceSetTreeItem::addData(const Surface& tSurface, const Annotation& tAnnotation, Qt3DCore::QEntity* p3DEntityParent)
{
    //Generate child items based on surface set input parameters
    bool state = false;

    QList<QStandardItem*> itemList = this->findChildren(BrainTreeModelItemTypes::HemisphereItem);

    bool hemiItemFound = false;

    //Search for already created hemi items and add source space data respectivley
    for(int j = 0; j<itemList.size(); j++) {
        BrainHemisphereTreeItem* pHemiItem = dynamic_cast<BrainHemisphereTreeItem*>(itemList.at(j));

        if(pHemiItem->data(BrainHemisphereTreeItemRoles::SurfaceHemi).toInt() == tSurface.hemi()) {
            hemiItemFound = true;

            if(tAnnotation.hemi() == tSurface.hemi()) {
                state = pHemiItem->addData(tSurface, tAnnotation, p3DEntityParent);
            } else {
                state = pHemiItem->addData(tSurface, Annotation(), p3DEntityParent);
            }
        }
    }

    if(!hemiItemFound) {
        //Item does not exist yet, create it here.
        BrainHemisphereTreeItem* pHemiItem = new BrainHemisphereTreeItem(BrainTreeModelItemTypes::HemisphereItem);

        if(tAnnotation.hemi() == tSurface.hemi()) {
            state = pHemiItem->addData(tSurface, tAnnotation, p3DEntityParent);
        } else {
            state = pHemiItem->addData(tSurface, Annotation(), p3DEntityParent);
        }

        QList<QStandardItem*> list;
        list<<new QStandardItem(pHemiItem->toolTip());
        list<<pHemiItem;
        this->appendRow(list);
    }

    return state;
}


//*************************************************************************************************************

bool BrainSurfaceSetTreeItem::addData(const MNESourceSpace& tSourceSpace, Qt3DCore::QEntity* p3DEntityParent)
{
    //Generate child items based on surface set input parameters
    bool state = false;

    QList<QStandardItem*> itemList = this->findChildren(BrainTreeModelItemTypes::HemisphereItem);

    //If there are more hemispheres in tSourceSpace than in the tree model
    bool hemiItemFound = false;

    //Search for already created hemi items and add source space data respectivley
    for(int i = 0; i < tSourceSpace.size(); i++) {
        for(int j = 0; j<itemList.size(); j++) {
            BrainHemisphereTreeItem* pHemiItem = dynamic_cast<BrainHemisphereTreeItem*>(itemList.at(j));

            if(pHemiItem->data(BrainHemisphereTreeItemRoles::SurfaceHemi).toInt() == i) {
                hemiItemFound = true;
                state = pHemiItem->addData(tSourceSpace[i], p3DEntityParent);
            }
        }

        if(!hemiItemFound) {
            //Item does not exist yet, create it here.
            BrainHemisphereTreeItem* pHemiItem = new BrainHemisphereTreeItem(BrainTreeModelItemTypes::HemisphereItem);

            state = pHemiItem->addData(tSourceSpace[i], p3DEntityParent);

            QList<QStandardItem*> list;
            list<<new QStandardItem(pHemiItem->toolTip());
            list<<pHemiItem;
            this->appendRow(list);
        }

        hemiItemFound = false;
    }

    return state;
}


//*************************************************************************************************************

void BrainSurfaceSetTreeItem::onCheckStateChanged(const Qt::CheckState& checkState)
{
    for(int i = 0; i<this->rowCount(); i++) {
        if(this->child(i)->isCheckable()) {
            this->child(i)->setCheckState(checkState);
        }
    }
}



