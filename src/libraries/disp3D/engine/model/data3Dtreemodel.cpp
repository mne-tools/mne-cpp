//=============================================================================================================
/**
 * @file     data3Dtreemodel.cpp
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Juan Garcia-Prieto <juangpc@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     May, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lars Debor, Gabriel B Motta, Juan Garcia-Prieto, Lorenz Esch. All rights reserved.
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "data3Dtreemodel.h"
#include "items/bem/bemtreeitem.h"
#include "items/sensorspace/sensorsettreeitem.h"
#include "items/subject/subjecttreeitem.h"
#include "items/freesurfer/fssurfacetreeitem.h"
#include "items/sourcespace/sourcespacetreeitem.h"
#include "items/measurement/measurementtreeitem.h"
#include "items/mri/mritreeitem.h"
#include "items/digitizer/digitizertreeitem.h"
#include "items/sensordata/sensordatatreeitem.h"
#include "3dhelpers/renderable3Dentity.h"

#include <inverse/dipoleFit/ecd_set.h>

#include <fs/surfaceset.h>
#include <fs/annotationset.h>

#include <fiff/fiff_dig_point_set.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <Qt3DCore/QEntity>
#include <QSurfaceFormat>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FSLIB;
using namespace MNELIB;
using namespace DISP3DLIB;
using namespace INVERSELIB;
using namespace CONNECTIVITYLIB;
using namespace Eigen;
using namespace FIFFLIB;

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

//=============================================================================================================

QVariant Data3DTreeModel::data(const QModelIndex& index,
                               int role) const
{
//    qDebug() << "Data3DTreeModel::data - index.column(): " << index.column();

//    if(index.column() == 1) {
//        QVariant data;
//        data.setValue(QString("test"));
//        return data;
//    }

    return QStandardItemModel::data(index, role);
}

//=============================================================================================================

int Data3DTreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    // Return 2 to activate item description in tree view
    return 1;
}

//=============================================================================================================

QVariant Data3DTreeModel::headerData(int section, Qt::Orientation orientation,
                                     int role) const
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

//=============================================================================================================

Qt::ItemFlags Data3DTreeModel::flags(const QModelIndex &index) const
{
    //Do not allow items from column 1 (0 based counting) to be edited
    if(index.column() == 1) {
        return Qt::ItemIsEnabled;
    }

    return QStandardItemModel::flags(index);
}

//=============================================================================================================

QList<FsSurfaceTreeItem*> Data3DTreeModel::addSurfaceSet(const QString& sSubject,
                                                         const QString& sMriSetName,
                                                         const SurfaceSet& surfaceSet,
                                                         const AnnotationSet& annotationSet)
{
    QList<FsSurfaceTreeItem*> returnItemList;

    for(int i = 0; i < surfaceSet.size(); ++i) {
        if(i < annotationSet.size()) {
            returnItemList.append(addSurface(sSubject, sMriSetName, surfaceSet[i], annotationSet[i]));
        } else {
            returnItemList.append(addSurface(sSubject, sMriSetName,surfaceSet[i], Annotation()));
        }
    }

    return returnItemList;
}

//=============================================================================================================

FsSurfaceTreeItem* Data3DTreeModel::addSurface(const QString& subject,
                                               const QString& sMriSetName,
                                               const Surface& surface,
                                               const Annotation &annotation)
{
    FsSurfaceTreeItem* pReturnItem = Q_NULLPTR;

    //Handle subject item
    SubjectTreeItem* pSubjectItem = addSubject(subject);

    //Find already existing MRI items and add the new data to the first search result
    QList<QStandardItem*> itemList = pSubjectItem->findChildren(sMriSetName);

    if(!itemList.isEmpty()) {
        MriTreeItem* pMriItem = dynamic_cast<MriTreeItem*>(itemList.first());
        pReturnItem = pMriItem->addData(surface, annotation, m_pModelEntity);
    } else {
        MriTreeItem* pMriItem = new MriTreeItem(Data3DTreeModelItemTypes::MriItem, sMriSetName);
        AbstractTreeItem::addItemWithDescription(pSubjectItem, pMriItem);
        pReturnItem = pMriItem->addData(surface, annotation, m_pModelEntity);
    }

    return pReturnItem;
}

//=============================================================================================================

QList<SourceSpaceTreeItem*> Data3DTreeModel::addSourceSpace(const QString& sSubject,
                                                            const QString& sMeasurementSetName,
                                                            const MNESourceSpace& sourceSpace)
{
    QList<SourceSpaceTreeItem*> pReturnItem;

    //Handle subject item
    SubjectTreeItem* pSubjectItem = addSubject(sSubject);

    //Find already existing surface items and add the new data to the first search result
    QList<QStandardItem*> itemList = pSubjectItem->findChildren(sMeasurementSetName);

    if(!itemList.isEmpty()) {
        MeasurementTreeItem* pMeasurementItem = dynamic_cast<MeasurementTreeItem*>(itemList.first());
        pReturnItem = pMeasurementItem->addData(sourceSpace, m_pModelEntity);
    } else {
        MeasurementTreeItem* pMeasurementItem = new MeasurementTreeItem(Data3DTreeModelItemTypes::MeasurementItem, sMeasurementSetName);
        AbstractTreeItem::addItemWithDescription(pSubjectItem, pMeasurementItem);
        pReturnItem = pMeasurementItem->addData(sourceSpace, m_pModelEntity);
    }

    return pReturnItem;
}

//=============================================================================================================

QList<SourceSpaceTreeItem*> Data3DTreeModel::addForwardSolution(const QString& sSubject,
                                                                const QString& sMeasurementSetName,
                                                                const MNEForwardSolution& forwardSolution)
{
    return this->addSourceSpace(sSubject, sMeasurementSetName, forwardSolution.src);
}

//=============================================================================================================

MneDataTreeItem* Data3DTreeModel::addSourceData(const QString& sSubject,
                                                const QString& sMeasurementSetName,
                                                const MNESourceEstimate& tSourceEstimate,
                                                const MNELIB::MNEForwardSolution& tForwardSolution,
                                                const FSLIB::SurfaceSet& tSurfSet,
                                                const FSLIB::AnnotationSet& tAnnotSet)
{
    bool bUseGPU = false;

    // Only support CPU support until we figured out the QBuffer memory problem when dealing with large matrices
//    if(QGLFormat::openGLVersionFlags() >= QGLFormat::OpenGL_Version_4_3) {
//        bUseGPU = true;
//        qDebug("Using compute shader version for 3D visualization.");
//    }

    MneDataTreeItem* pReturnItem = Q_NULLPTR;

    //Handle subject item
    SubjectTreeItem* pSubjectItem = addSubject(sSubject);

    //Find already existing surface items and add the new data to the first search result
    QList<QStandardItem*> itemList = pSubjectItem->findChildren(sMeasurementSetName);

    //Find the "set" items and add the sensor data as items
    if(!itemList.isEmpty() && (itemList.first()->type() == Data3DTreeModelItemTypes::MeasurementItem)) {
        if(MeasurementTreeItem* pMeasurementItem = dynamic_cast<MeasurementTreeItem*>(itemList.first())) {
            pReturnItem = pMeasurementItem->addData(tSourceEstimate,
                                                    tForwardSolution,
                                                    tSurfSet,
                                                    tAnnotSet,
                                                    m_pModelEntity,
                                                    bUseGPU);
        }
    } else {
        MeasurementTreeItem* pMeasurementItem = new MeasurementTreeItem(Data3DTreeModelItemTypes::MeasurementItem, sMeasurementSetName);
        AbstractTreeItem::addItemWithDescription(pSubjectItem, pMeasurementItem);
        pReturnItem = pMeasurementItem->addData(tSourceEstimate,
                                                tForwardSolution,
                                                tSurfSet,
                                                tAnnotSet,
                                                m_pModelEntity,
                                                bUseGPU);
    }

    return pReturnItem;
}

//=============================================================================================================

EcdDataTreeItem* Data3DTreeModel::addDipoleFitData(const QString& sSubject,
                                                   const QString& sSet,
                                                   const INVERSELIB::ECDSet& ecdSet)
{
    EcdDataTreeItem* pReturnItem = Q_NULLPTR;

    //Handle subject item
    SubjectTreeItem* pSubjectItem = addSubject(sSubject);

    //Find already existing surface items and add the new data to the first search result
    QList<QStandardItem*> itemList = pSubjectItem->findChildren(sSet);

    //Find the "set" items and add the dipole fits as items
    if(!itemList.isEmpty() && (itemList.first()->type() == Data3DTreeModelItemTypes::MeasurementItem)) {
        if(MeasurementTreeItem* pMeasurementItem = dynamic_cast<MeasurementTreeItem*>(itemList.first())) {
            pReturnItem = pMeasurementItem->addData(ecdSet, m_pModelEntity);
        }
    } else {
        MeasurementTreeItem* pMeasurementItem = new MeasurementTreeItem(Data3DTreeModelItemTypes::MeasurementItem, sSet);
        AbstractTreeItem::addItemWithDescription(pSubjectItem, pMeasurementItem);
        pReturnItem = pMeasurementItem->addData(ecdSet, m_pModelEntity);
    }

    return pReturnItem;
}

//=============================================================================================================

QList<NetworkTreeItem*> Data3DTreeModel::addConnectivityData(const QString& sSubject,
                                                             const QString& sMeasurementSetName,
                                                             const QList<Network>& networkData)
{
    QList<NetworkTreeItem*> returnList;

    for(int i = 0; i < networkData.size(); ++i) {
        returnList.append(addConnectivityData(sSubject,
                                              sMeasurementSetName,
                                              networkData.at(i)));
    }

    return returnList;
}

//=============================================================================================================

NetworkTreeItem* Data3DTreeModel::addConnectivityData(const QString& sSubject,
                                                      const QString& sMeasurementSetName,
                                                      const Network& networkData)
{
    NetworkTreeItem* pReturnItem = Q_NULLPTR;

    //Handle subject item
    SubjectTreeItem* pSubjectItem = addSubject(sSubject);

    //Find already existing surface items and add the new data to the first search result
    QList<QStandardItem*> itemList = pSubjectItem->findChildren(sMeasurementSetName);

    if(!itemList.isEmpty() && (itemList.first()->type() == Data3DTreeModelItemTypes::MeasurementItem)) {
        if(MeasurementTreeItem* pMeasurementItem = dynamic_cast<MeasurementTreeItem*>(itemList.first())) {
            pReturnItem = pMeasurementItem->addData(networkData, m_pModelEntity);
        }
    } else {
        MeasurementTreeItem* pMeasurementItem = new MeasurementTreeItem(Data3DTreeModelItemTypes::MeasurementItem, sMeasurementSetName);
        AbstractTreeItem::addItemWithDescription(pSubjectItem, pMeasurementItem);
        pReturnItem = pMeasurementItem->addData(networkData, m_pModelEntity);
    }

    return pReturnItem;
}

//=============================================================================================================

BemTreeItem* Data3DTreeModel::addBemData(const QString& sSubject,
                                         const QString& sBemSetName,
                                         const MNELIB::MNEBem& bem)
{
    BemTreeItem* pReturnItem = Q_NULLPTR;

    //Handle subject item
    SubjectTreeItem* pSubjectItem = addSubject(sSubject);

    //Find already existing surface items and add the new data to the first search result
    QList<QStandardItem*> itemList = pSubjectItem->findChildren(sBemSetName);

    if(!itemList.isEmpty() && (itemList.first()->type() == Data3DTreeModelItemTypes::BemItem)) {
        pReturnItem = dynamic_cast<BemTreeItem*>(itemList.first());
        pReturnItem->addData(bem, m_pModelEntity);
    } else {
        pReturnItem = new BemTreeItem(Data3DTreeModelItemTypes::BemItem, sBemSetName);
        AbstractTreeItem::addItemWithDescription(pSubjectItem, pReturnItem);
        pReturnItem->addData(bem, m_pModelEntity);
    }

    return pReturnItem;
}

//=============================================================================================================

SensorSetTreeItem* Data3DTreeModel::addMegSensorInfo(const QString& sSubject,
                                                     const QString& sSensorSetName,
                                                     const QList<FIFFLIB::FiffChInfo>& lChInfo,
                                                     const MNELIB::MNEBem& sensor,
                                                     const QStringList& bads)
{
    SensorSetTreeItem* pReturnItem = Q_NULLPTR;

    //Handle subject item
    SubjectTreeItem* pSubjectItem = addSubject(sSubject);

    //Find already existing surface items and add the new data to the first search result
    QList<QStandardItem*> itemList = pSubjectItem->findChildren(sSensorSetName);

    if(!itemList.isEmpty() && (itemList.first()->type() == Data3DTreeModelItemTypes::SensorSetItem)) {
        pReturnItem = dynamic_cast<SensorSetTreeItem*>(itemList.first());
        if(pReturnItem == Q_NULLPTR){
            qDebug()<<"Dynamic cast failed, returning null pointer";
        } else {
            pReturnItem->addData(sensor, lChInfo, "MEG", bads, m_pModelEntity);
        }
    } else {
        pReturnItem = new SensorSetTreeItem(Data3DTreeModelItemTypes::SensorSetItem, sSensorSetName);
        AbstractTreeItem::addItemWithDescription(pSubjectItem, pReturnItem);
        pReturnItem->addData(sensor, lChInfo, "MEG", bads, m_pModelEntity);
    }

    return pReturnItem;
}

//=============================================================================================================

SensorSetTreeItem* Data3DTreeModel::addEegSensorInfo(const QString& sSubject,
                                                     const QString& sSensorSetName,
                                                     const QList<FIFFLIB::FiffChInfo>& lChInfo,
                                                     const QStringList& bads)
{
    SensorSetTreeItem* pReturnItem = Q_NULLPTR;

    //Handle subject item
    SubjectTreeItem* pSubjectItem = addSubject(sSubject);

    //Find already existing surface items and add the new data to the first search result
    QList<QStandardItem*> itemList = pSubjectItem->findChildren(sSensorSetName);

    MNEBem tempBem = MNEBem();

    if(!itemList.isEmpty() && (itemList.first()->type() == Data3DTreeModelItemTypes::SensorSetItem)) {
        pReturnItem = dynamic_cast<SensorSetTreeItem*>(itemList.first());
        if(pReturnItem == Q_NULLPTR){
            qDebug() << "Dynamic cast failed, returning null pointer";
            pReturnItem = Q_NULLPTR;
        } else {
            pReturnItem->addData(tempBem, lChInfo, "EEG", bads, m_pModelEntity);
        }
    } else {
        pReturnItem = new SensorSetTreeItem(Data3DTreeModelItemTypes::SensorSetItem, sSensorSetName);
        AbstractTreeItem::addItemWithDescription(pSubjectItem, pReturnItem);
        pReturnItem->addData(tempBem, lChInfo, "EEG", bads, m_pModelEntity);
    }

    return pReturnItem;
}

//=============================================================================================================

DigitizerSetTreeItem* Data3DTreeModel::addDigitizerData(const QString& sSubject,
                                                        const QString& sMeasurementSetName,
                                                        const FIFFLIB::FiffDigPointSet& digitizer)
{
    DigitizerSetTreeItem* pReturnItem = Q_NULLPTR;

    //Handle subject item
    SubjectTreeItem* pSubjectItem = addSubject(sSubject);

    //Find already existing set items and add the new data to the first search result
    QList<QStandardItem*> itemList = pSubjectItem->findChildren(sMeasurementSetName);

    if(!itemList.isEmpty() && (itemList.first()->type() == Data3DTreeModelItemTypes::MeasurementItem)) {
        MeasurementTreeItem* pMeasurementItem = dynamic_cast<MeasurementTreeItem*>(itemList.first());
        pReturnItem = pMeasurementItem->addData(digitizer, m_pModelEntity);
    } else {
        MeasurementTreeItem* pMeasurementItem = new MeasurementTreeItem(Data3DTreeModelItemTypes::MeasurementItem, sMeasurementSetName);
        AbstractTreeItem::addItemWithDescription(pSubjectItem, pMeasurementItem);
        pReturnItem = pMeasurementItem->addData(digitizer, m_pModelEntity);
    }

    return pReturnItem;
}

//=============================================================================================================

SensorDataTreeItem* Data3DTreeModel::addSensorData(const QString& sSubject,
                                                   const QString& sMeasurementSetName,
                                                   const MatrixXd& matSensorData,
                                                   const MNEBemSurface& tBemSurface,
                                                   const FiffInfo& fiffInfo,
                                                   const QString& sDataType)
{
    bool bUseGPU = false;

    // Only support CPU support until we figured out the QBuffer memory problem when dealing with large matrices
//    if(QGLFormat::openGLVersionFlags() >= QGLFormat::OpenGL_Version_4_3) {
//        bUseGPU = true;
//        qDebug("Using compute shader version for 3D visualization.");
//    }

    SensorDataTreeItem* pReturnItem = Q_NULLPTR;

    //Handle subject item
    SubjectTreeItem* pSubjectItem = addSubject(sSubject);

    //Find already existing surface items and add the new data to the first search result
    QList<QStandardItem*> itemList = pSubjectItem->findChildren(sMeasurementSetName);

    //Find the "set" items and add the sensor data as items
    if(!itemList.isEmpty() && (itemList.first()->type() == Data3DTreeModelItemTypes::MeasurementItem)) {
        if(MeasurementTreeItem* pMeasurementItem = dynamic_cast<MeasurementTreeItem*>(itemList.first())) {
            pReturnItem = pMeasurementItem->addData(matSensorData,
                                                    tBemSurface,
                                                    fiffInfo,
                                                    sDataType,
                                                    m_pModelEntity,
                                                    bUseGPU);
        }
    } else {
        MeasurementTreeItem* pMeasurementItem = new MeasurementTreeItem(Data3DTreeModelItemTypes::MeasurementItem, sMeasurementSetName);
        AbstractTreeItem::addItemWithDescription(pSubjectItem, pMeasurementItem);
        pReturnItem = pMeasurementItem->addData(matSensorData,
                                                tBemSurface,
                                                fiffInfo,
                                                sDataType,
                                                m_pModelEntity,
                                                bUseGPU);
    }

    return pReturnItem;
}

//=============================================================================================================

QPointer<Qt3DCore::QEntity> Data3DTreeModel::getRootEntity()
{
    return m_pModelEntity;
}

//=============================================================================================================

SubjectTreeItem* Data3DTreeModel::addSubject(const QString& sSubject)
{
    SubjectTreeItem* pReturnItem= Q_NULLPTR;

    //Find the subject
    QList<QStandardItem*> itemSubjectList = this->findItems(sSubject);

    //If subject does not exist, create a new one
    if(itemSubjectList.size() == 0) {
        pReturnItem = new SubjectTreeItem(Data3DTreeModelItemTypes::SubjectItem, sSubject);
        itemSubjectList << pReturnItem;
        itemSubjectList << new QStandardItem(pReturnItem->toolTip());
        m_pRootItem->appendRow(itemSubjectList);
    } else {
        pReturnItem = dynamic_cast<SubjectTreeItem*>(itemSubjectList.first());
    }

    return pReturnItem;
}

//=============================================================================================================

void Data3DTreeModel::initMetatypes()
{
    //Init metatypes
    qRegisterMetaType<QVector<QVector<int> > >();
    qRegisterMetaType<QVector<int> >();

    qRegisterMetaType<QVector<Vector3f> >();
    qRegisterMetaType<QVector<Eigen::Vector3f> >();

    qRegisterMetaType<QList<FSLIB::Label> >();
    qRegisterMetaType<QList<Label> >();

    qRegisterMetaType<FIFFLIB::FiffInfo>();
    qRegisterMetaType<FiffInfo>();

    qRegisterMetaType<Eigen::MatrixX3i>();
    qRegisterMetaType<MatrixX3i>();

    qRegisterMetaType<Eigen::MatrixXd>();
    qRegisterMetaType<MatrixXd>();

    qRegisterMetaType<Eigen::MatrixX3f>();
    qRegisterMetaType<MatrixX3f>();

    qRegisterMetaType<Eigen::MatrixX4f>();
    qRegisterMetaType<MatrixX4f>();

    qRegisterMetaType<Eigen::VectorXf>();
    qRegisterMetaType<VectorXf>();

    qRegisterMetaType<Eigen::VectorXi>();
    qRegisterMetaType<VectorXi>();

    qRegisterMetaType<Eigen::VectorXd>();
    qRegisterMetaType<VectorXd>();

    qRegisterMetaType<Eigen::RowVectorXf>();
    qRegisterMetaType<RowVectorXf>();

    qRegisterMetaType<Eigen::Vector3f>();
    qRegisterMetaType<Vector3f>();

    qRegisterMetaType<Eigen::SparseMatrix<float> >();
    qRegisterMetaType<SparseMatrix<float> >();

    qRegisterMetaType<QSharedPointer<Eigen::SparseMatrix<float> > >();
    qRegisterMetaType<QSharedPointer<SparseMatrix<float> > >();
}
