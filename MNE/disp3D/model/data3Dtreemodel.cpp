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
#include "items/bem/bemtreeitem.h"
#include "items/subject/subjecttreeitem.h"
#include "items/freesurfer/fssurfacetreeitem.h"
#include "items/sourcespace/sourcespacetreeitem.h"
#include "items/measurement/measurementtreeitem.h"
#include "items/mri/mritreeitem.h"
#include "items/digitizer/digitizertreeitem.h"
#include "3dhelpers/renderable3Dentity.h"

#include <mne/mne_bem.h>

#include <inverse/dipoleFit/ecd_set.h>

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
using namespace INVERSELIB;
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

QList<FsSurfaceTreeItem*> Data3DTreeModel::addSurfaceSet(const QString& subject, const QString& sMriSetName, const SurfaceSet& tSurfaceSet, const AnnotationSet& tAnnotationSet)
{
    QList<FsSurfaceTreeItem*> returnItemList;

    for(int i = 0; i < tSurfaceSet.size(); ++i) {
        if(i < tAnnotationSet.size()) {
            returnItemList.append(addSurface(subject, sMriSetName, tSurfaceSet[i], tAnnotationSet[i]));
        } else {
            returnItemList.append(addSurface(subject, sMriSetName,tSurfaceSet[i], Annotation()));
        }
    }

    return returnItemList;
}


//*************************************************************************************************************

FsSurfaceTreeItem* Data3DTreeModel::addSurface(const QString& subject, const QString& sMriSetName, const Surface& tSurface, const Annotation &tAnnotation)
{
    FsSurfaceTreeItem* pReturnItem = Q_NULLPTR;

    //Handle subject item
    SubjectTreeItem* pSubjectItem = addSubject(subject);

    //Find already existing MRI items and add the new data to the first search result
    QList<QStandardItem*> itemList = pSubjectItem->findChildren(sMriSetName);

    if(!itemList.isEmpty()) {
        MriTreeItem* pMriItem = dynamic_cast<MriTreeItem*>(itemList.first());
        pReturnItem = pMriItem->addData(tSurface, tAnnotation, m_pModelEntity);
    } else {
        MriTreeItem* pMriItem = new MriTreeItem(Data3DTreeModelItemTypes::MriItem, sMriSetName);
        addItemWithDescription(pSubjectItem, pMriItem);
        pReturnItem = pMriItem->addData(tSurface, tAnnotation, m_pModelEntity);
    }

    return pReturnItem;
}


//*************************************************************************************************************

SourceSpaceTreeItem* Data3DTreeModel::addSourceSpace(const QString& subject, const QString& sMeasurementSetName, const MNESourceSpace& tSourceSpace)
{
    SourceSpaceTreeItem* pReturnItem = Q_NULLPTR;

    //Handle subject item
    SubjectTreeItem* pSubjectItem = addSubject(subject);

    //Find already existing surface items and add the new data to the first search result
    QList<QStandardItem*> itemList = pSubjectItem->findChildren(sMeasurementSetName);

    if(!itemList.isEmpty()) {
        MeasurementTreeItem* pMeasurementItem = dynamic_cast<MeasurementTreeItem*>(itemList.first());
        pReturnItem = pMeasurementItem->addData(tSourceSpace, m_pModelEntity);
    } else {
        MeasurementTreeItem* pMeasurementItem = new MeasurementTreeItem(Data3DTreeModelItemTypes::MeasurementItem, sMeasurementSetName);
        addItemWithDescription(pSubjectItem, pMeasurementItem);
        pReturnItem = pMeasurementItem->addData(tSourceSpace, m_pModelEntity);
    }

    return pReturnItem;
}


//*************************************************************************************************************

SourceSpaceTreeItem* Data3DTreeModel::addForwardSolution(const QString& subject, const QString& sMeasurementSetName, const MNEForwardSolution& tForwardSolution)
{
    return this->addSourceSpace(subject, sMeasurementSetName, tForwardSolution.src);
}


//*************************************************************************************************************

MneEstimateTreeItem* Data3DTreeModel::addSourceData(const QString& subject, const QString& sMeasurementSetName, const MNESourceEstimate& tSourceEstimate, const MNEForwardSolution& tForwardSolution)
{
    MneEstimateTreeItem* pReturnItem = Q_NULLPTR;

    //Handle subject item
    SubjectTreeItem* pSubjectItem = addSubject(subject);

    //Find already existing surface items and add the new data to the first search result
    QList<QStandardItem*> itemList = pSubjectItem->findChildren(sMeasurementSetName);

    //Find the "set" items and add the dipole fits as items
    if(!itemList.isEmpty() && (itemList.first()->type() == Data3DTreeModelItemTypes::MeasurementItem)) {
        if(MeasurementTreeItem* pMeasurementItem = dynamic_cast<MeasurementTreeItem*>(itemList.first())) {            
            //If measurement data has already been created but in conjunction with a different data type (i.e. connectivity, dipole fitting, etc.), do the connects here
            if(pMeasurementItem->findChildren(Data3DTreeModelItemTypes::MNEEstimateItem).isEmpty()) {
                connectMeasurementToMriItems(pSubjectItem, pMeasurementItem);
            }

            pReturnItem = pMeasurementItem->addData(tSourceEstimate, tForwardSolution);
        }
    } else {
        MeasurementTreeItem* pMeasurementItem = new MeasurementTreeItem(Data3DTreeModelItemTypes::MeasurementItem, sMeasurementSetName);
        addItemWithDescription(pSubjectItem, pMeasurementItem);
        pReturnItem = pMeasurementItem->addData(tSourceEstimate, tForwardSolution);

        connectMeasurementToMriItems(pSubjectItem, pMeasurementItem);
    }

    return pReturnItem;
}


//*************************************************************************************************************

EcdDataTreeItem* Data3DTreeModel::addDipoleFitData(const QString& subject, const QString& sMeasurementSetName, INVERSELIB::ECDSet::SPtr& pECDSet)
{
    EcdDataTreeItem* pReturnItem = Q_NULLPTR;

    //Handle subject item
    SubjectTreeItem* pSubjectItem = addSubject(subject);

    //Find already existing surface items and add the new data to the first search result
    QList<QStandardItem*> itemList = pSubjectItem->findChildren(sMeasurementSetName);

    //Find the "set" items and add the dipole fits as items
    if(!itemList.isEmpty() && (itemList.first()->type() == Data3DTreeModelItemTypes::MeasurementItem)) {
        if(MeasurementTreeItem* pMeasurementItem = dynamic_cast<MeasurementTreeItem*>(itemList.first())) {
            pReturnItem = pMeasurementItem->addData(pECDSet, m_pModelEntity);
        }
    } else {
        MeasurementTreeItem* pMeasurementItem = new MeasurementTreeItem(Data3DTreeModelItemTypes::MeasurementItem, sMeasurementSetName);
        addItemWithDescription(pSubjectItem, pMeasurementItem);
        pReturnItem = pMeasurementItem->addData(pECDSet, m_pModelEntity);
    }

    return pReturnItem;
}


//*************************************************************************************************************

NetworkTreeItem* Data3DTreeModel::addConnectivityData(const QString& subject, const QString& sMeasurementSetName, Network::SPtr pNetworkData)
{
    NetworkTreeItem* pReturnItem = Q_NULLPTR;

    //Handle subject item
    SubjectTreeItem* pSubjectItem = addSubject(subject);

    //Find already existing surface items and add the new data to the first search result
    QList<QStandardItem*> itemList = pSubjectItem->findChildren(sMeasurementSetName);

    if(!itemList.isEmpty() && (itemList.first()->type() == Data3DTreeModelItemTypes::MeasurementItem)) {
        if(MeasurementTreeItem* pMeasurementItem = dynamic_cast<MeasurementTreeItem*>(itemList.first())) {
            pReturnItem = pMeasurementItem->addData(pNetworkData, m_pModelEntity);
        }
    } else {
        MeasurementTreeItem* pMeasurementItem = new MeasurementTreeItem(Data3DTreeModelItemTypes::MeasurementItem, sMeasurementSetName);
        addItemWithDescription(pSubjectItem, pMeasurementItem);
        pReturnItem = pMeasurementItem->addData(pNetworkData, m_pModelEntity);
    }

    return pReturnItem;
}


//*************************************************************************************************************

BemTreeItem* Data3DTreeModel::addBemData(const QString& subject, const QString& sBemSetName, const MNELIB::MNEBem& tBem)
{
    BemTreeItem* pReturnItem = Q_NULLPTR;

    //Handle subject item
    SubjectTreeItem* pSubjectItem = addSubject(subject);

    //Find already existing surface items and add the new data to the first search result
    QList<QStandardItem*> itemList = pSubjectItem->findChildren(sBemSetName);

    if(!itemList.isEmpty() && (itemList.first()->type() == Data3DTreeModelItemTypes::BemItem)) {
        pReturnItem = dynamic_cast<BemTreeItem*>(itemList.first());
        pReturnItem->addData(tBem, m_pModelEntity);
    } else {
        pReturnItem = new BemTreeItem(Data3DTreeModelItemTypes::BemItem, sBemSetName);
        addItemWithDescription(pSubjectItem, pReturnItem);
        pReturnItem->addData(tBem, m_pModelEntity);
    }

    return pReturnItem;
}


//*************************************************************************************************************

DigitizerSetTreeItem* Data3DTreeModel::addDigitizerData(const QString& subject, const QString& sMeasurementSetName, const FIFFLIB::FiffDigPointSet& tDigitizer)
{
    DigitizerSetTreeItem* pReturnItem = Q_NULLPTR;

    //Handle subject item
    SubjectTreeItem* pSubjectItem = addSubject(subject);

    //Find already existing set items and add the new data to the first search result
    QList<QStandardItem*> itemList = pSubjectItem->findChildren(sMeasurementSetName);

    if(!itemList.isEmpty() && (itemList.first()->type() == Data3DTreeModelItemTypes::MeasurementItem)) {
        MeasurementTreeItem* pMeasurementItem = dynamic_cast<MeasurementTreeItem*>(itemList.first());
        pReturnItem = pMeasurementItem->addData(tDigitizer, m_pModelEntity);
    } else {
        MeasurementTreeItem* pMeasurementItem = new MeasurementTreeItem(Data3DTreeModelItemTypes::MeasurementItem, sMeasurementSetName);
        addItemWithDescription(pSubjectItem, pMeasurementItem);
        pReturnItem = pMeasurementItem->addData(tDigitizer, m_pModelEntity);
    }

    return pReturnItem;
}


//*************************************************************************************************************

QPointer<Qt3DCore::QEntity> Data3DTreeModel::getRootEntity()
{
    return m_pModelEntity;
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

SubjectTreeItem* Data3DTreeModel::addSubject(const QString& subject)
{
    SubjectTreeItem* pReturnItem= Q_NULLPTR;

    //Find the subject
    QList<QStandardItem*> itemSubjectList = this->findItems(subject);

    //If subject does not exist, create a new one
    if(itemSubjectList.size() == 0) {
        pReturnItem = new SubjectTreeItem(Data3DTreeModelItemTypes::SubjectItem, subject);
        itemSubjectList << pReturnItem;
        itemSubjectList << new QStandardItem(pReturnItem->toolTip());
        m_pRootItem->appendRow(itemSubjectList);
    } else {
        pReturnItem = dynamic_cast<SubjectTreeItem*>(itemSubjectList.first());
    }

    return pReturnItem;
}


//*************************************************************************************************************

void Data3DTreeModel::addItemWithDescription(QStandardItem* pItemParent, QStandardItem* pItemAdd)
{
    if(pItemParent && pItemAdd) {
        QList<QStandardItem*> list;
        list << pItemAdd;
        list << new QStandardItem(pItemAdd->toolTip());
        pItemParent->appendRow(list);
    }
}


//*************************************************************************************************************

void Data3DTreeModel::connectMeasurementToMriItems(SubjectTreeItem* pSubjectItem, MeasurementTreeItem* pMeasurementItem)
{
    //Connect mri item with all measurement tree items in case the real time color changes (i.e. rt source loc)
    //or the user changes the color origin
    QList<QStandardItem*> mriItemList = pSubjectItem->findChildren(Data3DTreeModelItemTypes::MriItem);

    for(int i = 0; i < mriItemList.size(); ++i) {
        if(MriTreeItem* pMriItem = dynamic_cast<MriTreeItem*>(mriItemList.at(i))) {
            connect(pMeasurementItem, &MeasurementTreeItem::rtVertColorChanged,
                pMriItem, &MriTreeItem::setRtVertColor);

            connect(pMriItem, &MriTreeItem::colorOriginChanged,
                pMeasurementItem, &MeasurementTreeItem::setColorOrigin);
        }
    }
}
