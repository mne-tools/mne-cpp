//=============================================================================================================
/**
* @file     data3Dtreemodel.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Data3DTreeModel class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "Data3DTreeModel.h"


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

Data3DTreeModel::Data3DTreeModel(QObject* parent, Qt3DCore::QEntity* parentEntity)
: QStandardItemModel(parent)
, m_pParentEntity(parentEntity)
{
    m_pRootItem = this->invisibleRootItem();
    m_pRootItem->setText("Loaded 3D Data");
}


//*************************************************************************************************************

Data3DTreeModel::~Data3DTreeModel()
{
    delete m_pRootItem;
}


//*************************************************************************************************************

QVariant Data3DTreeModel::data(const QModelIndex& index, int role) const
{
//    qDebug()<<"Data3DTreeModel::data - index.column(): "<<index.column();

//    if(index.column() == 1) {
//        QVariant data;
//        data.setValue(QString("test"));
//        return data;
//    }

    return QStandardItemModel::data(index, role);
}


//*************************************************************************************************************

int Data3DTreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 2;
}


//*************************************************************************************************************

QVariant Data3DTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        QVariant data;
        if(section == 0) {
            data.setValue(QString("Data"));
        } else if(section == 1) {
            data.setValue(QString("Description"));
        }

        return data;
    }

    return QVariant();
}


//*************************************************************************************************************

bool Data3DTreeModel::addData(const QString& subject, const QString& set, const SurfaceSet& tSurfaceSet, const AnnotationSet& tAnnotationSet)
{
    //Find the subject
    QList<QStandardItem*> itemSubjectList = this->findItems(subject);

    //If subject does not exist, create a new one
    if(itemSubjectList.size() == 0) {
        SubjectTreeItem* subjectItem = new SubjectTreeItem(SubjectTreeModelItemTypes::SubjectItem, subject);
        itemSubjectList << subjectItem;
        itemSubjectList<<new QStandardItem(subjectItem->toolTip());
        m_pRootItem->appendRow(itemSubjectList);
    }

    //Iterate through subject items and add new data respectivley
    bool state = false;

    for(int i = 0; i < itemSubjectList.size(); ++i) {
        //Check if it is really a subject tree item
        if((itemSubjectList.at(i)->type() == SubjectTreeModelItemTypes::SubjectItem)) {
            SubjectTreeItem* pSubjectItem = dynamic_cast<SubjectTreeItem*>(itemSubjectList.at(i));

            //Find already existing set items and add the new data to the first search result
            QList<QStandardItem*> itemList = pSubjectItem->findChildren(set);

            if(!itemList.isEmpty() && (itemList.at(0)->type() == BrainTreeModelItemTypes::SurfaceSetItem)) {
                BrainSurfaceSetTreeItem* pSurfaceSetItem = dynamic_cast<BrainSurfaceSetTreeItem*>(itemList.at(0));
                state = pSurfaceSetItem->addData(tSurfaceSet, tAnnotationSet, m_pParentEntity);
            } else {
                BrainSurfaceSetTreeItem* pSurfaceSetItem = new BrainSurfaceSetTreeItem(BrainTreeModelItemTypes::SurfaceSetItem, set);

                QList<QStandardItem*> list;
                list<<pSurfaceSetItem;
                list<<new QStandardItem(pSurfaceSetItem->toolTip());
                pSubjectItem->appendRow(list);

                state = pSurfaceSetItem->addData(tSurfaceSet, tAnnotationSet, m_pParentEntity);
            }
        }
    }

    return state;
}


//*************************************************************************************************************

bool Data3DTreeModel::addData(const QString& subject, const QString& set, const Surface& tSurface, const Annotation &tAnnotation)
{
    //Find the subject
    QList<QStandardItem*> itemSubjectList = this->findItems(subject);

    //If subject does not exist, create a new one
    if(itemSubjectList.size() == 0) {
        SubjectTreeItem* subjectItem = new SubjectTreeItem(SubjectTreeModelItemTypes::SubjectItem, subject);
        itemSubjectList << subjectItem;
        itemSubjectList<<new QStandardItem(subjectItem->toolTip());
        m_pRootItem->appendRow(itemSubjectList);
    }

    //Iterate through subject items and add new data respectivley
    bool state = false;

    for(int i = 0; i < itemSubjectList.size(); ++i) {
        //Check if it is really a subject tree item
        if((itemSubjectList.at(i)->type() == SubjectTreeModelItemTypes::SubjectItem)) {
            SubjectTreeItem* pSubjectItem = dynamic_cast<SubjectTreeItem*>(itemSubjectList.at(i));

            //Find already existing surface items and add the new data to the first search result
            QList<QStandardItem*> itemList = pSubjectItem->findChildren(set);

            if(!itemList.isEmpty() && (itemList.at(0)->type() == BrainTreeModelItemTypes::SurfaceSetItem)) {
                BrainSurfaceSetTreeItem* pSurfaceSetItem = dynamic_cast<BrainSurfaceSetTreeItem*>(itemList.at(0));
                state = pSurfaceSetItem->addData(tSurface, tAnnotation, m_pParentEntity);
            } else {
                BrainSurfaceSetTreeItem* pSurfaceSetItem = new BrainSurfaceSetTreeItem(BrainTreeModelItemTypes::SurfaceSetItem, set);

                QList<QStandardItem*> list;
                list<<pSurfaceSetItem;
                list<<new QStandardItem(pSurfaceSetItem->toolTip());
                pSubjectItem->appendRow(list);

                state = pSurfaceSetItem->addData(tSurface, tAnnotation, m_pParentEntity);
            }
        }
    }

    return state;
}


//*************************************************************************************************************

bool Data3DTreeModel::addData(const QString& subject, const QString& set, const MNESourceSpace& tSourceSpace)
{
    //Find the subject
    QList<QStandardItem*> itemSubjectList = this->findItems(subject);

    //If subject does not exist, create a new one
    if(itemSubjectList.size() == 0) {
        SubjectTreeItem* subjectItem = new SubjectTreeItem(SubjectTreeModelItemTypes::SubjectItem, subject);
        itemSubjectList << subjectItem;
        itemSubjectList<<new QStandardItem(subjectItem->toolTip());
        m_pRootItem->appendRow(itemSubjectList);
    }

    //Iterate through subject items and add new data respectivley
    bool state = false;

    for(int i = 0; i < itemSubjectList.size(); ++i) {
        //Check if it is really a subject tree item
        if((itemSubjectList.at(i)->type() == SubjectTreeModelItemTypes::SubjectItem)) {
            SubjectTreeItem* pSubjectItem = dynamic_cast<SubjectTreeItem*>(itemSubjectList.at(i));

            //Find already existing surface items and add the new data to the first search result
            QList<QStandardItem*> itemList = pSubjectItem->findChildren(set);

            if(!itemList.isEmpty() && (itemList.at(0)->type() == BrainTreeModelItemTypes::SurfaceSetItem)) {
                BrainSurfaceSetTreeItem* pSurfaceSetItem = dynamic_cast<BrainSurfaceSetTreeItem*>(itemList.at(0));
                state = pSurfaceSetItem->addData(tSourceSpace, m_pParentEntity);
            } else {
                BrainSurfaceSetTreeItem* pSurfaceSetItem = new BrainSurfaceSetTreeItem(BrainTreeModelItemTypes::SurfaceSetItem, set);

                QList<QStandardItem*> list;
                list<<pSurfaceSetItem;
                list<<new QStandardItem(pSurfaceSetItem->toolTip());
                pSubjectItem->appendRow(list);

                state = pSurfaceSetItem->addData(tSourceSpace, m_pParentEntity);
            }
        }
    }

    return state;
}


//*************************************************************************************************************

QList<BrainRTSourceLocDataTreeItem*> Data3DTreeModel::addData(const QString& subject, const QString& set, const MNESourceEstimate& tSourceEstimate, const MNEForwardSolution& tForwardSolution)
{
    QList<BrainRTSourceLocDataTreeItem*> returnList;

    //Find the subject
    QList<QStandardItem*> itemSubjectList = this->findItems(subject);

    //Iterate through subject items and add new data respectivley

    for(int i = 0; i < itemSubjectList.size(); ++i) {
        //Check if it is really a subject tree item
        if((itemSubjectList.at(i)->type() == SubjectTreeModelItemTypes::SubjectItem)) {
            SubjectTreeItem* pSubjectItem = dynamic_cast<SubjectTreeItem*>(itemSubjectList.at(i));

            //Find already existing surface items and add the new data to the first search result
            QList<QStandardItem*> itemList = pSubjectItem->findChildren(set);

            //Find the all the hemispheres of the set "set" and add the source estimates as items
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
        }
    }

    return returnList;
}
