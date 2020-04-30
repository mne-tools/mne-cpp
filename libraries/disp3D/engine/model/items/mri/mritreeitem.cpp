//=============================================================================================================
/**
 * @file     mritreeitem.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     November, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch. All rights reserved.
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
 * @brief    MriTreeItem class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mritreeitem.h"
#include "../freesurfer/fsannotationtreeitem.h"
#include "../freesurfer/fssurfacetreeitem.h"
#include "../hemisphere/hemispheretreeitem.h"

#include <fs/annotationset.h>
#include <fs/surfaceset.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;
using namespace DISP3DLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MriTreeItem::MriTreeItem(int iType,
                         const QString& text)
: AbstractTreeItem(iType, text)
{
    initItem();
}

//=============================================================================================================

void MriTreeItem::initItem()
{
    this->setEditable(false);
    this->setCheckable(true);
    this->setCheckState(Qt::Checked);
    this->setToolTip("MRI item");
}

//=============================================================================================================

QList<FsSurfaceTreeItem*> MriTreeItem::addData(const SurfaceSet& tSurfaceSet,
                                               const AnnotationSet& tAnnotationSet,
                                               Qt3DCore::QEntity* p3DEntityParent)
{
    //Generate child items based on surface set input parameters
    QList<FsSurfaceTreeItem*> returnItemList;

    //Search for already created hemi items and add source space data respectivley
    for(int i = 0; i < tSurfaceSet.size(); ++i) {
        if(i < tAnnotationSet.size()) {
            returnItemList.append(addData(tSurfaceSet[i], tAnnotationSet[i], p3DEntityParent));
        } else {
            returnItemList.append(addData(tSurfaceSet[i], Annotation(), p3DEntityParent));
        }
    }

    return returnItemList;
}

//=============================================================================================================

FsSurfaceTreeItem* MriTreeItem::addData(const Surface& tSurface,
                                        const Annotation& tAnnotation,
                                        Qt3DCore::QEntity* p3DEntityParent)
{
    //Generate child items based on surface set input parameters
    FsSurfaceTreeItem* pReturnItem = Q_NULLPTR;

    QList<QStandardItem*> itemList = this->findChildren(Data3DTreeModelItemTypes::HemisphereItem);

    bool hemiItemFound = false;

    //Search for already created hemi items and add data respectivley
    for(int j = 0; j < itemList.size(); ++j) {
        if(HemisphereTreeItem* pHemiItem = dynamic_cast<HemisphereTreeItem*>(itemList.at(j))) {
            if(pHemiItem->data(Data3DTreeModelItemRoles::SurfaceHemi).toInt() == tSurface.hemi()) {
                hemiItemFound = true;

                if(tAnnotation.hemi() == tSurface.hemi()) {
                    pReturnItem = pHemiItem->addData(tSurface, tAnnotation, p3DEntityParent);
                } else {
                    pReturnItem = pHemiItem->addData(tSurface, Annotation(), p3DEntityParent);
                }
            }
        }
    }

    //If item does not exist yet, create it here.
    if(!hemiItemFound) {
        HemisphereTreeItem* pHemiItem = new HemisphereTreeItem(Data3DTreeModelItemTypes::HemisphereItem);

        if(tAnnotation.hemi() == tSurface.hemi()) {
            pReturnItem = pHemiItem->addData(tSurface, tAnnotation, p3DEntityParent);
        } else {
            pReturnItem = pHemiItem->addData(tSurface, Annotation(), p3DEntityParent);
        }

        QList<QStandardItem*> list;
        list << pHemiItem;
        list << new QStandardItem(pHemiItem->toolTip());
        this->appendRow(list);
    }

    return pReturnItem;
}

