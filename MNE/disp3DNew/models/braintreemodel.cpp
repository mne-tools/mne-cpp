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
    rootData << "Loaded Data";
    m_pRootItem = new BrainTreeItem(rootData);
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

    return item->data(index.column());
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
        return m_pRootItem->data(section);

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

    BrainTreeItem* childItem = parentItem->child(row);
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
    BrainTreeItem* parentItem = childItem->parentItem();

    if (parentItem == m_pRootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}


//*************************************************************************************************************

bool BrainTreeModel::addFsData(const SurfaceSet::SPtr pSurfaceSet, const AnnotationSet::SPtr pAnnotationSet, Qt3DCore::QEntity *p3DEntityParent)
{
    qDebug()<<"BrainTreeModel::addFsData START";

    SurfaceSet tSurfaceSet = *pSurfaceSet.data();

    for(int i = 0; i< pSurfaceSet->size(); i++)
        if(i < pAnnotationSet->size())
            if((*pSurfaceSet.data())[i].hemi() == (*pAnnotationSet.data())[i].hemi())
                addFsData(Surface::SPtr((pSurfaceSet.data())[i]), Annotation::SPtr(&(*pAnnotationSet.data())[i]), p3DEntityParent);
//            else
//                addFsData(surfSPtrList.at(i), Annotation::SPtr(new Annotation()), p3DEntityParent);
//        else
//            addFsData(surfSPtrList.at(i), Annotation::SPtr(new Annotation()), p3DEntityParent);

//    QMap<qint32, Surface>::iterator i_surf;
//    QList<Surface::SPtr> surfSPtrList;
//    for (i_surf = pSurfaceSet->data().begin(); i_surf != pSurfaceSet->data().end(); ++i_surf) {
//        Surface::SPtr tmpSurfSPtr = Surface::SPtr(&i_surf.value());
//        surfSPtrList.append(tmpSurfSPtr);
//    }

//    QMap<qint32, Annotation>::iterator i_annot;
//    QList<Annotation::SPtr> annotSPtrList;
//    for (i_annot = pAnnotationSet->data().begin(); i_annot != pAnnotationSet->data().end(); ++i_annot) {
//        Annotation::SPtr tmpAnnotSPtr = Annotation::SPtr(&i_annot.value());
//        annotSPtrList<<tmpAnnotSPtr;
//    }

    qDebug()<<"BrainTreeModel::addFsData calling overloaded function";

//    for(int i = 0; i < surfSPtrList.size(); i++) {
//        if(i < annotSPtrList.size())
//            if(surfSPtrList.at(i)->hemi() == annotSPtrList.at(i)->hemi())
//                addFsData(surfSPtrList.at(i), annotSPtrList.at(i), p3DEntityParent);
//            else
//                addFsData(surfSPtrList.at(i), Annotation::SPtr(new Annotation()), p3DEntityParent);
//        else
//            addFsData(surfSPtrList.at(i), Annotation::SPtr(new Annotation()), p3DEntityParent);
//    }

    qDebug()<<"BrainTreeModel::addFsData END";

    return true;
}


//*************************************************************************************************************

bool BrainTreeModel::addFsData(const Surface::SPtr pSurface, const Annotation::SPtr pAnnotation, Qt3DCore::QEntity* p3DEntityParent)
{
    qDebug()<<"BrainTreeModel::addFsData overloaded function START";

    QList<QVariant> columnData;
    columnData<<"test";
    m_pRootItem->appendChild(new BrainTreeItem(columnData, m_pRootItem));

    qDebug()<<"BrainTreeModel::addFsData overloaded function END";

    return true;
}
