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

using namespace DISP3DNEWLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainTreeModel::BrainTreeModel(QObject *parent)
: QAbstractItemModel(parent)
{
    QList<QVariant> rootData;
    rootData << "Loaded 3D Data";
    m_pRootItem = new BrainTreeItem(rootData, BrainTreeItemModelRoles::RootItem, "RootItem");
}


//*************************************************************************************************************

BrainTreeModel::~BrainTreeModel()
{
    delete m_pRootItem;
}


//*************************************************************************************************************

int BrainTreeModel::rowCount(const QModelIndex &parent) const
{
    BrainTreeItem* parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = m_pRootItem;
    else
        parentItem = static_cast<BrainTreeItem*>(parent.internalPointer());

    return parentItem->childCount();
}


//*************************************************************************************************************

int BrainTreeModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<BrainTreeItem*>(parent.internalPointer())->columnCount();
    else
        return m_pRootItem->columnCount();
}


//*************************************************************************************************************

QVariant BrainTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    BrainTreeItem *item = static_cast<BrainTreeItem*>(index.internalPointer());

    return item->data(index.column(), role);
}


//*************************************************************************************************************

Qt::ItemFlags BrainTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return QAbstractItemModel::flags(index);
}


//*************************************************************************************************************

QVariant BrainTreeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return m_pRootItem->data(section, role);

    return QVariant();
}


//*************************************************************************************************************

QModelIndex BrainTreeModel::index(int row, int column, const QModelIndex &parent)
            const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    BrainTreeItem* parentItem;

    if (!parent.isValid())
        parentItem = m_pRootItem;
    else
        parentItem = static_cast<BrainTreeItem*>(parent.internalPointer());

    AbstractTreeItem* childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}


//*************************************************************************************************************

QModelIndex BrainTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    BrainTreeItem* childItem = static_cast<BrainTreeItem*>(index.internalPointer());
    AbstractTreeItem* parentItem = childItem->parentItem();

    if (parentItem == m_pRootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}


//*************************************************************************************************************

bool BrainTreeModel::addFsData(const SurfaceSet& tSurfaceSet, const AnnotationSet& tAnnotationSet, Qt3DCore::QEntity* p3DEntityParent)
{
    for(int i = 0; i < tSurfaceSet.size(); i++) {
        if(i < tAnnotationSet.size()) {
            if(tAnnotationSet[i].hemi() == tSurfaceSet[i].hemi()) {
                addFsData(tSurfaceSet[i], tAnnotationSet[i], p3DEntityParent);
            } else {
                addFsData(tSurfaceSet[i], Annotation(), p3DEntityParent);
            }
        } else {
            addFsData(tSurfaceSet[i], Annotation(), p3DEntityParent);
        }
    }

    return true;
}


//*************************************************************************************************************

bool BrainTreeModel::addFsData(const Surface &tSurface, const Annotation &tAnnotation, Qt3DCore::QEntity* p3DEntityParent)
{
//    QList<QVariant> lDataVariant;

//    //Top level brain tree item
//    lDataVariant<<tSurface.fileName();
//    BrainTreeItem* pBrainTreeItemTop = new BrainTreeItem(lDataVariant, BrainTreeItem::SurfName, m_pRootItem);
//    m_pRootItem->appendChild(pBrainTreeItemTop);
//    lDataVariant.clear();

//    //Actual data of the top level brain item
//    //FilePath
//    lDataVariant<<tSurface.filePath();
//    BrainTreeItem* pBrainTreeItemPath = new BrainTreeItem(lDataVariant, BrainTreeItem::FilePath, pBrainTreeItemTop);
//    pBrainTreeItemTop->appendChild(pBrainTreeItemPath);
//    lDataVariant.clear();

//    //AnnotName
//    lDataVariant<<(tAnnotation.fileName().isEmpty() ? "unspecified" : tAnnotation.fileName());
//    BrainTreeItem* pBrainTreeItemAnnotName = new BrainTreeItem(lDataVariant, BrainTreeItem::AnnotName, pBrainTreeItemTop);
//    pBrainTreeItemTop->appendChild(pBrainTreeItemAnnotName);
//    lDataVariant.clear();

//    //SurfType
//    lDataVariant<<tSurface.surf();
//    BrainTreeItem* pBrainTreeItemSurfType = new BrainTreeItem(lDataVariant, BrainTreeItem::SurfType, pBrainTreeItemTop);
//    pBrainTreeItemTop->appendChild(pBrainTreeItemSurfType);
//    lDataVariant.clear();

//    //Hemi
//    lDataVariant<<tSurface.hemi();
//    BrainTreeItem* pBrainTreeItemHemi = new BrainTreeItem(lDataVariant, BrainTreeItem::Hemi, pBrainTreeItemTop);
//    pBrainTreeItemTop->appendChild(pBrainTreeItemHemi);
//    lDataVariant.clear();

//    //ColorSulci
//    lDataVariant<<QColor(50,50,50);
//    BrainTreeItem* pBrainTreeItemColorSulci = new BrainTreeItem(lDataVariant, BrainTreeItem::ColorSulci, pBrainTreeItemTop);
//    pBrainTreeItemTop->appendChild(pBrainTreeItemColorSulci);
//    lDataVariant.clear();

//    //ColorGyri
//    lDataVariant<<QColor(125,125,125);
//    BrainTreeItem* pBrainTreeItemColorGyri = new BrainTreeItem(lDataVariant, BrainTreeItem::ColorGyri, pBrainTreeItemTop);
//    pBrainTreeItemTop->appendChild(pBrainTreeItemColorGyri);
//    lDataVariant.clear();

//    //Renderable3DEntity
//    Renderable3DEntity* pRenderable3DEntity = new Renderable3DEntity(tSurface.rr(), tSurface.nn(), tSurface.tris(), -tSurface.offset(), p3DEntityParent);
//    QVariant var;
//    var.setValue(pRenderable3DEntity);
//    lDataVariant<<var;
//    BrainTreeItem* pBrainTreeItem3DEntity = new BrainTreeItem(lDataVariant, BrainTreeItem::Renderable3DEntity, pBrainTreeItemTop);
//    pBrainTreeItemTop->appendChild(pBrainTreeItem3DEntity);
//    lDataVariant.clear();

    return true;
}
