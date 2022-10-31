//=============================================================================================================
/**
 * @file     bemtreeitem.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     May, 2016
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
 * @brief    BemTreeItem class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bemtreeitem.h"
#include "bemsurfacetreeitem.h"

#include <mne/mne_bem.h>

#include <fiff/fiff_coord_trans.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;
using namespace MNELIB;
using namespace DISP3DLIB;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BemTreeItem::BemTreeItem(int iType, const QString& text)
: AbstractTreeItem(iType, text)
{
    initItem();
}

//=============================================================================================================

void BemTreeItem::setTransform(const Qt3DCore::QTransform& transform)
{
    if(m_pRenderable3DEntity) {
        m_pRenderable3DEntity->setTransform(transform);
    }
}

//=============================================================================================================

void BemTreeItem::setTransform(const FiffCoordTrans& transform,
                               bool bApplyInverse)
{
    if(m_pRenderable3DEntity) {
        m_pRenderable3DEntity->setTransform(transform, bApplyInverse);
    }
}

//=============================================================================================================

void BemTreeItem::applyTransform(const Qt3DCore::QTransform& transform)
{
    if(m_pRenderable3DEntity) {
        m_pRenderable3DEntity->applyTransform(transform);
    }
}

//=============================================================================================================

void BemTreeItem::applyTransform(const FiffCoordTrans& transform,
                                 bool bApplyInverse)
{
    if(m_pRenderable3DEntity) {
        m_pRenderable3DEntity->applyTransform(transform, bApplyInverse);
    }
}

//=============================================================================================================

void BemTreeItem::initItem()
{
    this->setEditable(false);
    this->setCheckable(true);
    this->setCheckState(Qt::Checked);
    this->setToolTip("BEM item");
}

//=============================================================================================================

void BemTreeItem::addData(const MNEBem &tBem, Qt3DCore::QEntity* p3DEntityParent)
{
    if(!m_pRenderable3DEntity) {
        m_pRenderable3DEntity = new Renderable3DEntity(p3DEntityParent);
    }

    //Generate child items based on BEM input parameters
    for(int i = 0; i < tBem.size(); ++i) {
        QString sBemSurfName;
        sBemSurfName = QString("%1").arg(tBem[i].id);
        BemSurfaceTreeItem* pSurfItem = new BemSurfaceTreeItem(m_pRenderable3DEntity, Data3DTreeModelItemTypes::BemSurfaceItem, sBemSurfName);
        pSurfItem->addData(tBem[i]);

        QList<QStandardItem*> list;
        list << pSurfItem;
        list << new QStandardItem(pSurfItem->toolTip());
        this->appendRow(list);
    }
}

