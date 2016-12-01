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

#include "data3Dtreemodel.h"
#include "bem/bemtreeitem.h"
#include "subject/subjecttreeitem.h"
#include "brain/brainsurfacetreeitem.h"
#include "brain/brainsurfacesettreeitem.h"
#include "digitizer/digitizertreeitem.h"
#include "common/renderable3Dentity.h"

#include <mne/mne_bem.h>
#include <fs/surfaceset.h>
#include <fs/annotationset.h>
#include <fiff/fiff_dig_point_set.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

#include <Qt3DCore/QEntity>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;
using namespace MNELIB;
using namespace DISP3DLIB;
using namespace CONNECTIVITYLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Data3DTreeModel::Data3DTreeModel(QObject* parent)
: QStandardItemModel(parent)
, m_pModelEntity(new Qt3DCore::QEntity())
{
    m_pRootItem = this->invisibleRootItem();
    m_pRootItem->setText("Loaded 3D Data");

    initMetatypes();
}


//*************************************************************************************************************

Data3DTreeModel::~Data3DTreeModel()
{
    //delete m_pRootItem;
}


//*************************************************************************************************************

void Data3DTreeModel::initMetatypes()
{
    //Init metatypes
    qRegisterMetaType<QByteArray>();
    qRegisterMetaType<QPair<QByteArray, QByteArray> >();

    qRegisterMetaType<Eigen::MatrixX3i>();
    qRegisterMetaType<Eigen::MatrixXd>();
    qRegisterMetaType<Eigen::MatrixX3f>();
    qRegisterMetaType<Eigen::VectorXf>();
    qRegisterMetaType<Eigen::VectorXi>();
    qRegisterMetaType<Eigen::VectorXd>();
    qRegisterMetaType<Eigen::RowVectorXf>();
    qRegisterMetaType<Eigen::Vector3f>();

    qRegisterMetaType<MatrixX3i>();
    qRegisterMetaType<MatrixXd>();
    qRegisterMetaType<MatrixX3f>();
    qRegisterMetaType<VectorXf>();
    qRegisterMetaType<VectorXi>();
    qRegisterMetaType<VectorXd>();
    qRegisterMetaType<RowVectorXf>();
    qRegisterMetaType<Vector3f>();
}


//*************************************************************************************************************

QVariant Data3DTreeModel::data(const QModelIndex& index, int role) const
{
//    qDebug() << "Data3DTreeModel::data - index.column(): " << index.column();

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

Qt::ItemFlags Data3DTreeModel::flags(const QModelIndex &index) const
{
    //Do not allow items from column 1 (0 based counting) to be edited
    if(index.column() == 1) {
        return Qt::ItemIsEnabled;
    }

    return QStandardItemModel::flags(index);
}


//*************************************************************************************************************

bool Data3DTreeModel::addSurfaceSet(const QString& subject, const QString& set, const SurfaceSet& tSurfaceSet, const AnnotationSet& tAnnotationSet)
{
    //Find the subject
    QList<QStandardItem*> itemSubjectList = this->findItems(subject);

    //If subject does not exist, create a new one
    if(itemSubjectList.size() == 0) {
        SubjectTreeItem* subjectItem = new SubjectTreeItem(Data3DTreeModelItemTypes::SubjectItem, subject);
        itemSubjectList << subjectItem;
        itemSubjectList << new QStandardItem(subjectItem->toolTip());
        m_pRootItem->appendRow(itemSubjectList);
    }

    //Iterate through subject items and add new data respectivley
    bool state = false;

    for(int i = 0; i < itemSubjectList.size(); ++i) {
        //Check if it is really a subject tree item
        if((itemSubjectList.at(i)->type() == Data3DTreeModelItemTypes::SubjectItem)) {
            SubjectTreeItem* pSubjectItem = dynamic_cast<SubjectTreeItem*>(itemSubjectList.at(i));

            //Find already existing set items and add the new data to the first search result
            QList<QStandardItem*> itemList = pSubjectItem->findChildren(set);

            if(!itemList.isEmpty() && (itemList.at(0)->type() == Data3DTreeModelItemTypes::SurfaceSetItem)) {
                BrainSurfaceSetTreeItem* pSurfaceSetItem = dynamic_cast<BrainSurfaceSetTreeItem*>(itemList.at(0));
                state = pSurfaceSetItem->addData(tSurfaceSet, tAnnotationSet, m_pModelEntity);
            } else {
                BrainSurfaceSetTreeItem* pSurfaceSetItem = new BrainSurfaceSetTreeItem(Data3DTreeModelItemTypes::SurfaceSetItem, set);

                QList<QStandardItem*> list;
                list << pSurfaceSetItem;
                list << new QStandardItem(pSurfaceSetItem->toolTip());
                pSubjectItem->appendRow(list);

                state = pSurfaceSetItem->addData(tSurfaceSet, tAnnotationSet, m_pModelEntity);
            }
        }
    }

    return state;
}


//*************************************************************************************************************

bool Data3DTreeModel::addSurface(const QString& subject, const QString& set, const Surface& tSurface, const Annotation &tAnnotation)
{
    //Find the subject
    QList<QStandardItem*> itemSubjectList = this->findItems(subject);

    //If subject does not exist, create a new one
    if(itemSubjectList.size() == 0) {
        SubjectTreeItem* subjectItem = new SubjectTreeItem(Data3DTreeModelItemTypes::SubjectItem, subject);
        itemSubjectList << subjectItem;
        itemSubjectList << new QStandardItem(subjectItem->toolTip());
        m_pRootItem->appendRow(itemSubjectList);
    }

    //Iterate through subject items and add new data respectivley
    bool state = false;

    for(int i = 0; i < itemSubjectList.size(); ++i) {
        //Check if it is really a subject tree item
        if((itemSubjectList.at(i)->type() == Data3DTreeModelItemTypes::SubjectItem)) {
            SubjectTreeItem* pSubjectItem = dynamic_cast<SubjectTreeItem*>(itemSubjectList.at(i));

            //Find already existing surface items and add the new data to the first search result
            QList<QStandardItem*> itemList = pSubjectItem->findChildren(set);

            if(!itemList.isEmpty() && (itemList.at(0)->type() == Data3DTreeModelItemTypes::SurfaceSetItem)) {
                BrainSurfaceSetTreeItem* pSurfaceSetItem = dynamic_cast<BrainSurfaceSetTreeItem*>(itemList.at(0));
                state = pSurfaceSetItem->addData(tSurface, tAnnotation, m_pModelEntity);
            } else {
                BrainSurfaceSetTreeItem* pSurfaceSetItem = new BrainSurfaceSetTreeItem(Data3DTreeModelItemTypes::SurfaceSetItem, set);

                QList<QStandardItem*> list;
                list << pSurfaceSetItem;
                list << new QStandardItem(pSurfaceSetItem->toolTip());
                pSubjectItem->appendRow(list);

                state = pSurfaceSetItem->addData(tSurface, tAnnotation, m_pModelEntity);
            }
        }
    }

    return state;
}


//*************************************************************************************************************

bool Data3DTreeModel::addSourceSpace(const QString& subject, const QString& set, const MNESourceSpace& tSourceSpace)
{
    //Find the subject
    QList<QStandardItem*> itemSubjectList = this->findItems(subject);

    //If subject does not exist, create a new one
    if(itemSubjectList.size() == 0) {
        SubjectTreeItem* subjectItem = new SubjectTreeItem(Data3DTreeModelItemTypes::SubjectItem, subject);
        itemSubjectList << subjectItem;
        itemSubjectList << new QStandardItem(subjectItem->toolTip());
        m_pRootItem->appendRow(itemSubjectList);
    }

    //Iterate through subject items and add new data respectivley
    bool state = false;

    for(int i = 0; i < itemSubjectList.size(); ++i) {
        //Check if it is really a subject tree item
        if((itemSubjectList.at(i)->type() == Data3DTreeModelItemTypes::SubjectItem)) {
            SubjectTreeItem* pSubjectItem = dynamic_cast<SubjectTreeItem*>(itemSubjectList.at(i));

            //Find already existing surface items and add the new data to the first search result
            QList<QStandardItem*> itemList = pSubjectItem->findChildren(set);

            if(!itemList.isEmpty() && (itemList.at(0)->type() == Data3DTreeModelItemTypes::SurfaceSetItem)) {
                BrainSurfaceSetTreeItem* pSurfaceSetItem = dynamic_cast<BrainSurfaceSetTreeItem*>(itemList.at(0));
                state = pSurfaceSetItem->addData(tSourceSpace, m_pModelEntity);
            } else {
                BrainSurfaceSetTreeItem* pSurfaceSetItem = new BrainSurfaceSetTreeItem(Data3DTreeModelItemTypes::SurfaceSetItem, set);

                QList<QStandardItem*> list;
                list << pSurfaceSetItem;
                list << new QStandardItem(pSurfaceSetItem->toolTip());
                pSubjectItem->appendRow(list);

                state = pSurfaceSetItem->addData(tSourceSpace, m_pModelEntity);
            }
        }
    }

    return state;
}


//*************************************************************************************************************

bool Data3DTreeModel::addForwardSolution(const QString& subject, const QString& set, const MNEForwardSolution& tForwardSolution)
{
    return this->addSourceSpace(subject, set, tForwardSolution.src);
}


//*************************************************************************************************************

QList<BrainRTSourceLocDataTreeItem*> Data3DTreeModel::addSourceData(const QString& subject, const QString& set, const MNESourceEstimate& tSourceEstimate, const MNEForwardSolution& tForwardSolution)
{
    QList<BrainRTSourceLocDataTreeItem*> returnList;

    //Find the subject
    QList<QStandardItem*> itemSubjectList = this->findItems(subject);

    //Iterate through subject items and add new data respectivley

    for(int i = 0; i < itemSubjectList.size(); ++i) {
        //Check if it is really a subject tree item
        if((itemSubjectList.at(i)->type() == Data3DTreeModelItemTypes::SubjectItem)) {
            SubjectTreeItem* pSubjectItem = dynamic_cast<SubjectTreeItem*>(itemSubjectList.at(i));

            //Find already existing surface items and add the new data to the first search result
            QList<QStandardItem*> itemList = pSubjectItem->findChildren(set);

            //Find the "set" items and add the source estimates as items
            if(!itemList.isEmpty()) {
                for(int i = 0; i < itemList.size(); ++i) {
                    if(itemList.at(i)->type() == Data3DTreeModelItemTypes::SurfaceSetItem) {
                        if(BrainSurfaceSetTreeItem* pSetItem = dynamic_cast<BrainSurfaceSetTreeItem*>(itemList.at(i))) {
                            returnList.append(pSetItem->addData(tSourceEstimate, tForwardSolution));
                        }
                    }
                }
            }

//            //Find the all the hemispheres of the set "set" and add the source estimates as items
//            if(!itemList.isEmpty()) {
//                for(int i = 0; i<itemList.size(); i++) {
//                    for(int j = 0; j<itemList.at(i)->rowCount(); j++) {
//                        if(itemList.at(i)->child(j,0)->type() == Data3DTreeModelItemTypes::HemisphereItem) {
//                            BrainHemisphereTreeItem* pHemiItem = dynamic_cast<BrainHemisphereTreeItem*>(itemList.at(i)->child(j,0));
//                            returnList.append(pHemiItem->addData(tSourceEstimate, tForwardSolution));
//                        }
//                    }
//                }
//            }
        }
    }

    return returnList;
}


//*************************************************************************************************************

QList<BrainRTConnectivityDataTreeItem*> Data3DTreeModel::addConnectivityData(const QString& subject, const QString& set, Network::SPtr pNetworkData)
{
    QList<BrainRTConnectivityDataTreeItem*> returnList;

    //Find the subject
    QList<QStandardItem*> itemSubjectList = this->findItems(subject);

    //Iterate through subject items and add new data respectivley

    for(int i = 0; i < itemSubjectList.size(); ++i) {
        //Check if it is really a subject tree item
        if((itemSubjectList.at(i)->type() == Data3DTreeModelItemTypes::SubjectItem)) {
            SubjectTreeItem* pSubjectItem = dynamic_cast<SubjectTreeItem*>(itemSubjectList.at(i));

            //Find already existing surface items and add the new data to the first search result
            QList<QStandardItem*> itemList = pSubjectItem->findChildren(set);

            if(!itemList.isEmpty() && (itemList.at(0)->type() == Data3DTreeModelItemTypes::SurfaceSetItem)) {
                BrainSurfaceSetTreeItem* pSurfaceSetItem = dynamic_cast<BrainSurfaceSetTreeItem*>(itemList.at(0));
                returnList.append(pSurfaceSetItem->addData(pNetworkData, m_pModelEntity));
            } else {
                BrainSurfaceSetTreeItem* pSurfaceSetItem = new BrainSurfaceSetTreeItem(Data3DTreeModelItemTypes::SurfaceSetItem, set);

                QList<QStandardItem*> list;
                list << pSurfaceSetItem;
                list << new QStandardItem(pSurfaceSetItem->toolTip());
                pSubjectItem->appendRow(list);

                returnList.append(pSurfaceSetItem->addData(pNetworkData, m_pModelEntity));
            }

//            //Find the "set" items and add the source estimates as items
//            if(!itemList.isEmpty()) {
//                for(int i = 0; i<itemList.size(); i++) {
//                    if(itemList.at(i)->type() == Data3DTreeModelItemTypes::SurfaceSetItem) {
//                        if(BrainSurfaceSetTreeItem* pSetItem = dynamic_cast<BrainSurfaceSetTreeItem*>(itemList.at(i))) {
//                            returnList.append(pSetItem->addData(pNetworkData, m_pModelEntity));
//                        }
//                    }
//                }
//            }

//            //Find the all the hemispheres of the set "set" and add the source estimates as items
//            if(!itemList.isEmpty()) {
//                for(int i = 0; i<itemList.size(); i++) {
//                    for(int j = 0; j<itemList.at(i)->rowCount(); j++) {
//                        if(itemList.at(i)->child(j,0)->type() == Data3DTreeModelItemTypes::HemisphereItem) {
//                            BrainHemisphereTreeItem* pHemiItem = dynamic_cast<BrainHemisphereTreeItem*>(itemList.at(i)->child(j,0));
//                            returnList.append(pHemiItem->addData(tSourceEstimate, tForwardSolution));
//                        }
//                    }
//                }
//            }
        }
    }

    return returnList;
}


//*************************************************************************************************************

bool Data3DTreeModel::addBemData(const QString& subject, const QString& set, const MNELIB::MNEBem& tBem)
{
    //Find the subject
    QList<QStandardItem*> itemSubjectList = this->findItems(subject);

    //If subject does not exist, create a new one
    if(itemSubjectList.size() == 0) {
        SubjectTreeItem* subjectItem = new SubjectTreeItem(Data3DTreeModelItemTypes::SubjectItem, subject);
        itemSubjectList << subjectItem;
        itemSubjectList << new QStandardItem(subjectItem->toolTip());
        m_pRootItem->appendRow(itemSubjectList);
    }

    //Iterate through subject items and add new data respectivley
    bool state = false;

    for(int i = 0; i < itemSubjectList.size(); ++i) {
        //Check if it is really a subject tree item
        if((itemSubjectList.at(i)->type() == Data3DTreeModelItemTypes::SubjectItem)) {
            SubjectTreeItem* pSubjectItem = dynamic_cast<SubjectTreeItem*>(itemSubjectList.at(i));

            //Find already existing surface items and add the new data to the first search result
            QList<QStandardItem*> itemList = pSubjectItem->findChildren(set);

            if(!itemList.isEmpty() && (itemList.at(0)->type() == Data3DTreeModelItemTypes::BemItem)) {
                BemTreeItem* pBemItem = dynamic_cast<BemTreeItem*>(itemList.at(0));
                state = pBemItem->addData(tBem, m_pModelEntity);
            } else {
                BemTreeItem* pBemItem = new BemTreeItem(Data3DTreeModelItemTypes::BemItem, set);

                QList<QStandardItem*> list;
                list << pBemItem;
                list << new QStandardItem(pBemItem->toolTip());
                pSubjectItem->appendRow(list);

                state = pBemItem->addData(tBem, m_pModelEntity);
            }
        }
    }

    return state;
}


//*************************************************************************************************************

bool Data3DTreeModel::addDigitizerData(const QString& subject, const QString& set, const FIFFLIB::FiffDigPointSet& tDigitizer)
{
    //Find the subject
    QList<QStandardItem*> itemSubjectList = this->findItems(subject);

    //If subject does not exist, create a new one
    if(itemSubjectList.size() == 0) {
        SubjectTreeItem* subjectItem = new SubjectTreeItem(Data3DTreeModelItemTypes::SubjectItem, subject);
        itemSubjectList << subjectItem;
        itemSubjectList << new QStandardItem(subjectItem->toolTip());
        m_pRootItem->appendRow(itemSubjectList);
    }

    //Iterate through subject items and add new data respectivley
    bool state = false;

    for(int i = 0; i < itemSubjectList.size(); ++i) {
        //Check if it is really a subject tree item
        if((itemSubjectList.at(i)->type() == Data3DTreeModelItemTypes::SubjectItem)) {
            SubjectTreeItem* pSubjectItem = dynamic_cast<SubjectTreeItem*>(itemSubjectList.at(i));

            //Find already existing set items and add the new data to the first search result
            QList<QStandardItem*> itemList = pSubjectItem->findChildren(set);

            if(!itemList.isEmpty() && (itemList.at(0)->type() == Data3DTreeModelItemTypes::SurfaceSetItem)) {
                BrainSurfaceSetTreeItem* pSurfaceSetItem = dynamic_cast<BrainSurfaceSetTreeItem*>(itemList.at(0));
                state = pSurfaceSetItem->addData(tDigitizer, m_pModelEntity);
            } else {
                BrainSurfaceSetTreeItem* pSurfaceSetItem = new BrainSurfaceSetTreeItem(Data3DTreeModelItemTypes::SurfaceSetItem, set);

                QList<QStandardItem*> list;
                list << pSurfaceSetItem;
                list << new QStandardItem(pSurfaceSetItem->toolTip());
                pSubjectItem->appendRow(list);

                state = pSurfaceSetItem->addData(tDigitizer, m_pModelEntity);
            }
        }
    }

    return state;
}


//*************************************************************************************************************

QPointer<Qt3DCore::QEntity> Data3DTreeModel::getRootEntity()
{
    return m_pModelEntity;
}

