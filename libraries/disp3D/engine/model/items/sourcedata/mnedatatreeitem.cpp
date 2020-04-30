//=============================================================================================================
/**
 * @file     mnedatatreeitem.cpp
 * @author   Juan Garcia-Prieto <juangpc@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     December, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Juan Garcia-Prieto, Lorenz Esch. All rights reserved.
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
 * @brief    MneDataTreeItem class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mnedatatreeitem.h"
#include "../../workers/rtSourceLoc/rtsourcedatacontroller.h"
#include "../common/metatreeitem.h"
#include "../common/abstractmeshtreeitem.h"
#include "../common/gpuinterpolationitem.h"
#include "../freesurfer/fssurfacetreeitem.h"
#include "../../3dhelpers/custommesh.h"
#include "../../materials/pervertexphongalphamaterial.h"

#include <mne/mne_sourceestimate.h>
#include <mne/mne_forwardsolution.h>
#include <fs/surfaceset.h>
#include <fs/annotationset.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector3D>
#include <Qt3DCore/QEntity>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace MNELIB;
using namespace DISP3DLIB;
using namespace FSLIB;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MneDataTreeItem::MneDataTreeItem(int iType,
                                         const QString &text,
                                         bool bUseGPU)
: AbstractTreeItem(iType, text)
, m_bIsDataInit(false)
, m_bUseGPU(bUseGPU)
{
    initItem();
}

//=============================================================================================================

MneDataTreeItem::~MneDataTreeItem()
{
    m_pRtSourceDataController->deleteLater();
}

//=============================================================================================================

void MneDataTreeItem::initItem()
{    
    this->setCheckable(true);
    this->setCheckState(Qt::Checked);
    this->setEditable(false);
    this->setToolTip("MNE Data item");

    //Add items
    QList<QStandardItem*> list;
    QVariant data;

    MetaTreeItem* pItemStreamStatus = new MetaTreeItem(MetaTreeItemTypes::StreamStatus, "Stream data on/off");
    connect(pItemStreamStatus, &MetaTreeItem::checkStateChanged,
            this, &MneDataTreeItem::onCheckStateWorkerChanged);
    list << pItemStreamStatus;
    list << new QStandardItem(pItemStreamStatus->toolTip());
    this->appendRow(list);
    pItemStreamStatus->setCheckable(true);
    pItemStreamStatus->setCheckState(Qt::Unchecked);

    data.setValue(false);
    pItemStreamStatus->setData(data, MetaTreeItemRoles::StreamStatus);

    QString defaultVisualizationType = "Interpolation based";
    MetaTreeItem* pItemVisuaizationType = new MetaTreeItem(MetaTreeItemTypes::VisualizationType, defaultVisualizationType);
    connect(pItemVisuaizationType, &MetaTreeItem::dataChanged,
            this, &MneDataTreeItem::onVisualizationTypeChanged);
    list.clear();
    list << pItemVisuaizationType;
    list << new QStandardItem(pItemVisuaizationType->toolTip());
    this->appendRow(list);
    data.setValue(defaultVisualizationType);
    pItemVisuaizationType->setData(data, MetaTreeItemRoles::VisualizationType);

    MetaTreeItem* pItemColormapType = new MetaTreeItem(MetaTreeItemTypes::ColormapType, "Hot");
    connect(pItemColormapType, &MetaTreeItem::dataChanged,
            this, &MneDataTreeItem::onColormapTypeChanged);
    list.clear();
    list << pItemColormapType;
    list << new QStandardItem(pItemColormapType->toolTip());
    this->appendRow(list);
    data.setValue(QString("Hot"));
    pItemColormapType->setData(data, MetaTreeItemRoles::ColormapType);

    MetaTreeItem* pItemSourceLocNormValue = new MetaTreeItem(MetaTreeItemTypes::DataThreshold, "0.0,5.5,15");
    connect(pItemSourceLocNormValue, &MetaTreeItem::dataChanged,
            this, &MneDataTreeItem::onDataThresholdChanged);
    list.clear();
    list << pItemSourceLocNormValue;
    list << new QStandardItem(pItemSourceLocNormValue->toolTip());
    this->appendRow(list);
    data.setValue(QVector3D(0.0,5.5,15));
    pItemSourceLocNormValue->setData(data, MetaTreeItemRoles::DataThreshold);

    MetaTreeItem *pItemStreamingInterval = new MetaTreeItem(MetaTreeItemTypes::StreamingTimeInterval, "17");
    connect(pItemStreamingInterval, &MetaTreeItem::dataChanged,
            this, &MneDataTreeItem::onTimeIntervalChanged);
    list.clear();
    list << pItemStreamingInterval;
    list << new QStandardItem(pItemStreamingInterval->toolTip());
    this->appendRow(list);
    data.setValue(17);
    pItemStreamingInterval->setData(data, MetaTreeItemRoles::StreamingTimeInterval);

    MetaTreeItem *pItemLoopedStreaming = new MetaTreeItem(MetaTreeItemTypes::LoopedStreaming, "Loop last data");
    connect(pItemLoopedStreaming, &MetaTreeItem::checkStateChanged,
            this, &MneDataTreeItem::onCheckStateLoopedStateChanged);
    pItemLoopedStreaming->setCheckable(true);
    pItemLoopedStreaming->setCheckState(Qt::Checked);
    list.clear();
    list << pItemLoopedStreaming;
    list << new QStandardItem(pItemLoopedStreaming->toolTip());
    this->appendRow(list);

    MetaTreeItem *pItemAveragedStreaming = new MetaTreeItem(MetaTreeItemTypes::NumberAverages, "17");
    connect(pItemAveragedStreaming, &MetaTreeItem::dataChanged,
            this, &MneDataTreeItem::onNumberAveragesChanged);
    list.clear();
    list << pItemAveragedStreaming;
    list << new QStandardItem(pItemAveragedStreaming->toolTip());
    this->appendRow(list);
    data.setValue(1);
    pItemAveragedStreaming->setData(data, MetaTreeItemRoles::NumberAverages);

//    MetaTreeItem *pItemCancelDistance = new MetaTreeItem(MetaTreeItemTypes::CancelDistance, "0.05");
//    list.clear();
//    list << pItemCancelDistance;
//    list << new QStandardItem(pItemCancelDistance->toolTip());
//    this->appendRow(list);
//    data.setValue(0.05);
//    pItemCancelDistance->setData(data, MetaTreeItemRoles::CancelDistance);
//    connect(pItemCancelDistance, &MetaTreeItem::dataChanged,
//            this, &MneDataTreeItem::onCancelDistanceChanged);

    MetaTreeItem* pInterpolationFunction = new MetaTreeItem(MetaTreeItemTypes::InterpolationFunction, "Cubic");
    connect(pInterpolationFunction, &MetaTreeItem::dataChanged,
            this, &MneDataTreeItem::onInterpolationFunctionChanged);
    list.clear();
    list << pInterpolationFunction;
    list << new QStandardItem(pInterpolationFunction->toolTip());
    this->appendRow(list);
    data.setValue(QString("Cubic"));
    pInterpolationFunction->setData(data, MetaTreeItemRoles::InterpolationFunction);
}

//=============================================================================================================

void MneDataTreeItem::initData(const MNEForwardSolution& tForwardSolution,
                               const SurfaceSet& tSurfSet,
                               const AnnotationSet& tAnnotSet,
                               Qt3DCore::QEntity* p3DEntityParent)
{   
    if(tForwardSolution.src.size() < 2 || tAnnotSet.size() < 2 || tSurfSet.size() < 2) {
        qDebug() << "MneDataTreeItem::initData - Two hemisphere were not found. Check input.";
        qDebug() << "MneDataTreeItem::initData - tForwardSolution.src.size(): "<<tForwardSolution.src.size();
        qDebug() << "MneDataTreeItem::initData - tSurfSet.size(): "<<tSurfSet.size();
        qDebug() << "MneDataTreeItem::initData - tAnnotSet.size(): "<<tAnnotSet.size();
        return;
    }

    //Set data based on clusterd or full source space
    bool isClustered = tForwardSolution.src[0].isClustered();

    VectorXi clustVertNoTemp, clustVertNoLeft, clustVertNoRight;

    for(int i = 0; i < tForwardSolution.src.size(); ++i) {
        if(isClustered) {
            //When clustered source space, the idx no's are the annotation labels. Take the .cluster_info.centroidVertno instead.
            clustVertNoTemp.resize(tForwardSolution.src[i].cluster_info.centroidVertno.size());
            for(int j = 0; j < clustVertNoTemp.rows(); ++j) {
                clustVertNoTemp(j) = tForwardSolution.src[i].cluster_info.centroidVertno.at(j);
            }
        } else {
            clustVertNoTemp = tForwardSolution.src[i].vertno;
        }

        if(i == 0) {
            clustVertNoLeft = clustVertNoTemp;
        } else if (i == 1) {
            clustVertNoRight = clustVertNoTemp;
        }
    }

    //Add meta information as item children
    QList<QStandardItem*> list;
    QVariant data;

    QString sIsClustered = isClustered ? "Clustered" : "Full";
    MetaTreeItem* pItemSourceSpaceType = new MetaTreeItem(MetaTreeItemTypes::SourceSpaceType, sIsClustered);
    pItemSourceSpaceType->setEditable(false);
    list.clear();
    list << pItemSourceSpaceType;
    list << new QStandardItem(pItemSourceSpaceType->toolTip());
    this->appendRow(list);
    data.setValue(sIsClustered);
    pItemSourceSpaceType->setData(data, MetaTreeItemRoles::SourceSpaceType);

    //Process annotation data for patch based visualization
    QList<FSLIB::Label> qListLabelsLeft, qListLabelsRight;
    QList<RowVector4i> qListLabelRGBAs;

    VectorXi vecLabelIdsLeftHemi;
    VectorXi vecLabelIdsRightHemi;

    vecLabelIdsLeftHemi = tAnnotSet[0].getLabelIds();
    vecLabelIdsRightHemi = tAnnotSet[1].getLabelIds();
    tAnnotSet[0].toLabels(tSurfSet[0], qListLabelsLeft, qListLabelRGBAs);
    tAnnotSet[1].toLabels(tSurfSet[1], qListLabelsRight, qListLabelRGBAs);

    //set rt data corresponding to the hemisphere
    if(!m_pRtSourceDataController) {
        m_pRtSourceDataController = new RtSourceDataController();
    }

    //Create InterpolationItems for CPU or GPU usage
    if(m_bUseGPU) {
        if(!m_pInterpolationItemLeftGPU)
        {
            m_pInterpolationItemLeftGPU = new GpuInterpolationItem(p3DEntityParent,
                                                                   Data3DTreeModelItemTypes::GpuInterpolationItem,
                                                                   QStringLiteral("3D Plot - Left"));

            m_pInterpolationItemLeftGPU->initData(tSurfSet[0].rr(),
                                                  tSurfSet[0].nn(),
                                                  tSurfSet[0].tris());

            m_pInterpolationItemLeftGPU->setPosition(QVector3D(-tSurfSet[0].offset()(0),
                                                               -tSurfSet[0].offset()(1),
                                                               -tSurfSet[0].offset()(2)));

            QList<QStandardItem*> list;
            list << m_pInterpolationItemLeftGPU;
            list << new QStandardItem(m_pInterpolationItemLeftGPU->toolTip());
            this->appendRow(list);

            m_pInterpolationItemLeftGPU->setAlpha(1.0f);
        }

        if(!m_pInterpolationItemRightGPU)
        {
            m_pInterpolationItemRightGPU = new GpuInterpolationItem(p3DEntityParent,
                                                                    Data3DTreeModelItemTypes::GpuInterpolationItem,
                                                                    QStringLiteral("3D Plot - Right"));

            m_pInterpolationItemRightGPU->initData(tSurfSet[1].rr(),
                                                   tSurfSet[1].nn(),
                                                   tSurfSet[1].tris());

            m_pInterpolationItemRightGPU->setPosition(QVector3D(-tSurfSet[1].offset()(0),
                                                               -tSurfSet[1].offset()(1),
                                                               -tSurfSet[1].offset()(2)));

            QList<QStandardItem*> list;
            list << m_pInterpolationItemRightGPU;
            list << new QStandardItem(m_pInterpolationItemRightGPU->toolTip());
            this->appendRow(list);

            m_pInterpolationItemRightGPU->setAlpha(1.0f);
        }

        m_pRtSourceDataController->setStreamSmoothedData(false);

        connect(m_pRtSourceDataController.data(), &RtSourceDataController::newInterpolationMatrixLeftAvailable,
                        this, &MneDataTreeItem::onNewInterpolationMatrixLeftAvailable);

        connect(m_pRtSourceDataController.data(), &RtSourceDataController::newInterpolationMatrixRightAvailable,
                        this, &MneDataTreeItem::onNewInterpolationMatrixRightAvailable);

        connect(m_pRtSourceDataController.data(), &RtSourceDataController::newRtRawDataAvailable,
                this, &MneDataTreeItem::onNewRtRawData);
    } else {
        if(!m_pInterpolationItemLeftCPU) {
            m_pInterpolationItemLeftCPU = new AbstractMeshTreeItem(p3DEntityParent,
                                                            Data3DTreeModelItemTypes::AbstractMeshItem,
                                                            QStringLiteral("3D Plot - Left"));

            m_pInterpolationItemLeftCPU->setMeshData(tSurfSet[0].rr(),
                                                     tSurfSet[0].nn(),
                                                     tSurfSet[0].tris(),
                                                     FsSurfaceTreeItem::createCurvatureVertColor(tSurfSet[0].curv()),
                                                     Qt3DRender::QGeometryRenderer::Triangles);

            m_pInterpolationItemLeftCPU->setPosition(QVector3D(-tSurfSet[0].offset()(0),
                                                               -tSurfSet[0].offset()(1),
                                                               -tSurfSet[0].offset()(2)));

            QList<QStandardItem*> list;
            list << m_pInterpolationItemLeftCPU;
            list << new QStandardItem(m_pInterpolationItemLeftCPU->toolTip());
            this->appendRow(list);

            m_pInterpolationItemLeftCPU->setAlpha(1.0f);

            //Set material to enable sorting
            QPointer<PerVertexPhongAlphaMaterial> pBemMaterial = new PerVertexPhongAlphaMaterial(true);
            m_pInterpolationItemLeftCPU->setMaterial(pBemMaterial);
        }

        if(!m_pInterpolationItemRightCPU) {
            m_pInterpolationItemRightCPU = new AbstractMeshTreeItem(p3DEntityParent,
                                                            Data3DTreeModelItemTypes::AbstractMeshItem,
                                                            QStringLiteral("3D Plot - Right"));

            m_pInterpolationItemRightCPU->setMeshData(tSurfSet[1].rr(),
                                                      tSurfSet[1].nn(),
                                                      tSurfSet[1].tris(),
                                                      FsSurfaceTreeItem::createCurvatureVertColor(tSurfSet[1].curv()),
                                                      Qt3DRender::QGeometryRenderer::Triangles);

            m_pInterpolationItemRightCPU->setPosition(QVector3D(-tSurfSet[1].offset()(0),
                                                                -tSurfSet[1].offset()(1),
                                                                -tSurfSet[1].offset()(2)));

            QList<QStandardItem*> list;
            list << m_pInterpolationItemRightCPU;
            list << new QStandardItem(m_pInterpolationItemRightCPU->toolTip());
            this->appendRow(list);

            m_pInterpolationItemRightCPU->setAlpha(1.0f);

            //Set material to enable sorting
            QPointer<PerVertexPhongAlphaMaterial> pBemMaterial = new PerVertexPhongAlphaMaterial(true);
            m_pInterpolationItemRightCPU->setMaterial(pBemMaterial);
        }

        connect(m_pRtSourceDataController.data(), &RtSourceDataController::newRtSmoothedDataAvailable,
                this, &MneDataTreeItem::onNewRtSmoothedDataAvailable);
    }

    m_pRtSourceDataController->setInterpolationInfo(tForwardSolution.src[0].rr,
                                                    tForwardSolution.src[1].rr,
                                                    tForwardSolution.src[0].neighbor_vert,
                                                    tForwardSolution.src[1].neighbor_vert,
                                                    clustVertNoLeft,
                                                    clustVertNoRight);

    m_pRtSourceDataController->setSurfaceColor(FsSurfaceTreeItem::createCurvatureVertColor(tSurfSet[0].curv()),
                                               FsSurfaceTreeItem::createCurvatureVertColor(tSurfSet[1].curv()));

    m_pRtSourceDataController->setAnnotationInfo(vecLabelIdsLeftHemi,
                                                 vecLabelIdsRightHemi,
                                                 qListLabelsLeft,
                                                 qListLabelsRight,
                                                 clustVertNoLeft,
                                                 clustVertNoRight);

    m_bIsDataInit = true;
}

//=============================================================================================================

void MneDataTreeItem::addData(const MNESourceEstimate& tSourceEstimate)
{
    if(!m_bIsDataInit) {
        qDebug() << "MneDataTreeItem::addData - Item has not been initialized yet!";
        return;
    }

    //Set new data into item's data. The set data is for example needed in the delegate to calculate the histogram.
    QVariant data;
    data.setValue(tSourceEstimate.data);
    this->setData(data, Data3DTreeModelItemRoles::Data);

    // Only draw activation if item is checked
    if(m_pRtSourceDataController && this->checkState() == Qt::Checked) {
        m_pRtSourceDataController->addData(tSourceEstimate.data);
    }
}

//=============================================================================================================

void MneDataTreeItem::setLoopState(bool state)
{
    QList<QStandardItem*> lItems = this->findChildren(MetaTreeItemTypes::LoopedStreaming);

    for(int i = 0; i < lItems.size(); i++) {
        if(MetaTreeItem* pAbstractItem = dynamic_cast<MetaTreeItem*>(lItems.at(i))) {
            pAbstractItem->setCheckState(state == true ? Qt::Checked : Qt::Unchecked);
            QVariant data;
            data.setValue(state);
            pAbstractItem->setData(data, MetaTreeItemRoles::LoopedStreaming);
        }
    }
}

//=============================================================================================================

void MneDataTreeItem::setStreamingState(bool state)
{
    QList<QStandardItem*> lItems = this->findChildren(MetaTreeItemTypes::StreamStatus);

    for(int i = 0; i < lItems.size(); i++) {
        if(MetaTreeItem* pAbstractItem = dynamic_cast<MetaTreeItem*>(lItems.at(i))) {
            pAbstractItem->setCheckState(state == true ? Qt::Checked : Qt::Unchecked);
            QVariant data;
            data.setValue(state);
            pAbstractItem->setData(data, MetaTreeItemRoles::StreamStatus);
        }
    }
}

//=============================================================================================================

void MneDataTreeItem::setTimeInterval(int iMSec)
{
    QList<QStandardItem*> lItems = this->findChildren(MetaTreeItemTypes::StreamingTimeInterval);

    for(int i = 0; i < lItems.size(); i++) {
        if(MetaTreeItem* pAbstractItem = dynamic_cast<MetaTreeItem*>(lItems.at(i))) {
            QVariant data;
            data.setValue(iMSec);
            pAbstractItem->setData(data, MetaTreeItemRoles::StreamingTimeInterval);
            pAbstractItem->setData(data, Qt::DisplayRole);
        }
    }
}

//=============================================================================================================

void MneDataTreeItem::setNumberAverages(int iNumberAverages)
{
    QList<QStandardItem*> lItems = this->findChildren(MetaTreeItemTypes::NumberAverages);

    for(int i = 0; i < lItems.size(); i++) {
        if(MetaTreeItem* pAbstractItem = dynamic_cast<MetaTreeItem*>(lItems.at(i))) {
            QVariant data;
            data.setValue(iNumberAverages);
            pAbstractItem->setData(data, MetaTreeItemRoles::NumberAverages);
            pAbstractItem->setData(data, Qt::DisplayRole);
        }
    }
}

//=============================================================================================================

void MneDataTreeItem::setColormapType(const QString& sColormap)
{
    QList<QStandardItem*> lItems = this->findChildren(MetaTreeItemTypes::ColormapType);

    for(int i = 0; i < lItems.size(); i++) {
        if(MetaTreeItem* pAbstractItem = dynamic_cast<MetaTreeItem*>(lItems.at(i))) {
            QVariant data;
            data.setValue(sColormap);
            pAbstractItem->setData(data, MetaTreeItemRoles::ColormapType);
            pAbstractItem->setData(data, Qt::DisplayRole);
        }
    }
}

//=============================================================================================================

void MneDataTreeItem::setVisualizationType(const QString& sVisualizationType)
{
    QList<QStandardItem*> lItems = this->findChildren(MetaTreeItemTypes::VisualizationType);

    for(int i = 0; i < lItems.size(); i++) {
        if(MetaTreeItem* pAbstractItem = dynamic_cast<MetaTreeItem*>(lItems.at(i))) {
            QVariant data;
            data.setValue(sVisualizationType);
            pAbstractItem->setData(data, MetaTreeItemRoles::VisualizationType);
            pAbstractItem->setData(data, Qt::DisplayRole);
        }
    }
}

//=============================================================================================================

void MneDataTreeItem::setThresholds(const QVector3D& vecThresholds)
{
    QList<QStandardItem*> lItems = this->findChildren(MetaTreeItemTypes::DataThreshold);

    for(int i = 0; i < lItems.size(); i++) {
        if(MetaTreeItem* pAbstractItem = dynamic_cast<MetaTreeItem*>(lItems.at(i))) {
            QVariant data;
            data.setValue(vecThresholds);
            pAbstractItem->setData(data, MetaTreeItemRoles::DataThreshold);

            QString sTemp = QString("%1,%2,%3").arg(vecThresholds.x()).arg(vecThresholds.y()).arg(vecThresholds.z());
            data.setValue(sTemp);
            pAbstractItem->setData(data, Qt::DisplayRole);
        }
    }
}

//=============================================================================================================

//void MneDataTreeItem::setCancelDistance(double dCancelDist)
//{
//    QList<QStandardItem*> lItems = this->findChildren(MetaTreeItemTypes::CancelDistance);

//    for(int i = 0; i < lItems.size(); i++) {
//        if(MetaTreeItem* pAbstractItem = dynamic_cast<MetaTreeItem*>(lItems.at(i))) {
//            QVariant data;
//            data.setValue(dCancelDist);
//            pAbstractItem->setData(data, MetaTreeItemRoles::CancelDistance);
//            pAbstractItem->setData(data, Qt::DisplayRole);
//        }
//    }
//}

//=============================================================================================================

void MneDataTreeItem::setInterpolationFunction(const QString &sInterpolationFunction)
{
    QList<QStandardItem*> lItems = this->findChildren(MetaTreeItemTypes::InterpolationFunction);

    for(int i = 0; i < lItems.size(); i++) {
        if(MetaTreeItem* pAbstractItem = dynamic_cast<MetaTreeItem*>(lItems.at(i))) {
            QVariant data;
            data.setValue(sInterpolationFunction);
            pAbstractItem->setData(data, MetaTreeItemRoles::InterpolationFunction);
            pAbstractItem->setData(data, Qt::DisplayRole);
        }
    }
}

//=============================================================================================================

void MneDataTreeItem::setSFreq(const double dSFreq)
{
    if(m_pRtSourceDataController) {
        m_pRtSourceDataController->setSFreq(dSFreq);
    }
}

//=============================================================================================================

void MneDataTreeItem::setAlpha(float fAlpha)
{
    if(m_pInterpolationItemLeftCPU) {
        m_pInterpolationItemLeftCPU->setAlpha(fAlpha);
    }
    if(m_pInterpolationItemLeftGPU) {
        m_pInterpolationItemLeftGPU->setAlpha(fAlpha);
    }
    if(m_pInterpolationItemRightCPU) {
        m_pInterpolationItemRightCPU->setAlpha(fAlpha);
    }
    if(m_pInterpolationItemRightGPU) {
        m_pInterpolationItemRightGPU->setAlpha(fAlpha);
    }
}

//=============================================================================================================

void MneDataTreeItem::setTransform(const Qt3DCore::QTransform& transform)
{
    if(m_pInterpolationItemLeftCPU) {
        m_pInterpolationItemLeftCPU->setTransform(transform);
    }

    if(m_pInterpolationItemLeftGPU) {
        m_pInterpolationItemLeftGPU->setTransform(transform);
    }

    if(m_pInterpolationItemRightCPU) {
        m_pInterpolationItemRightCPU->setTransform(transform);
    }

    if(m_pInterpolationItemRightGPU) {
        m_pInterpolationItemRightGPU->setTransform(transform);
    }
}

//=============================================================================================================

void MneDataTreeItem::setTransform(const FiffCoordTrans& transform, bool bApplyInverse)
{
    if(m_pInterpolationItemLeftCPU) {
        m_pInterpolationItemLeftCPU->setTransform(transform, bApplyInverse);
    }

    if(m_pInterpolationItemLeftGPU) {
        m_pInterpolationItemLeftGPU->setTransform(transform, bApplyInverse);
    }

    if(m_pInterpolationItemRightCPU) {
        m_pInterpolationItemRightCPU->setTransform(transform, bApplyInverse);
    }

    if(m_pInterpolationItemRightGPU) {
        m_pInterpolationItemRightGPU->setTransform(transform, bApplyInverse);
    }
}

//=============================================================================================================

void MneDataTreeItem::applyTransform(const Qt3DCore::QTransform& transform)
{
    if(m_pInterpolationItemLeftCPU) {
        m_pInterpolationItemLeftCPU->applyTransform(transform);
    }

    if(m_pInterpolationItemLeftGPU) {
        m_pInterpolationItemLeftGPU->applyTransform(transform);
    }

    if(m_pInterpolationItemRightCPU) {
        m_pInterpolationItemRightCPU->applyTransform(transform);
    }

    if(m_pInterpolationItemRightGPU) {
        m_pInterpolationItemRightGPU->applyTransform(transform);
    }
}

//=============================================================================================================

void MneDataTreeItem::applyTransform(const FiffCoordTrans& transform, bool bApplyInverse)
{
    if(m_pInterpolationItemLeftCPU) {
        m_pInterpolationItemLeftCPU->applyTransform(transform, bApplyInverse);
    }

    if(m_pInterpolationItemLeftGPU) {
        m_pInterpolationItemLeftGPU->applyTransform(transform, bApplyInverse);
    }

    if(m_pInterpolationItemRightCPU) {
        m_pInterpolationItemRightCPU->applyTransform(transform, bApplyInverse);
    }

    if(m_pInterpolationItemRightGPU) {
        m_pInterpolationItemRightGPU->applyTransform(transform, bApplyInverse);
    }
}

//=============================================================================================================

void MneDataTreeItem::onCheckStateWorkerChanged(const Qt::CheckState& checkState)
{
    if(m_pRtSourceDataController) {
        if(checkState == Qt::Checked) {
            m_pRtSourceDataController->setStreamingState(true);
        } else if(checkState == Qt::Unchecked) {
            m_pRtSourceDataController->setStreamingState(false);
        }
    }
}

//=============================================================================================================

void MneDataTreeItem::onNewRtSmoothedDataAvailable(const Eigen::MatrixX4f &matColorMatrixLeftHemi,
                                                   const Eigen::MatrixX4f &matColorMatrixRightHemi)
{
    if(m_pInterpolationItemLeftCPU) {
        m_pInterpolationItemLeftCPU->setVertColor(matColorMatrixLeftHemi);
    }

    if(m_pInterpolationItemRightCPU) {
        m_pInterpolationItemRightCPU->setVertColor(matColorMatrixRightHemi);
    }
}

//=============================================================================================================

void MneDataTreeItem::onNewInterpolationMatrixLeftAvailable(QSharedPointer<Eigen::SparseMatrix<float> > pMatInterpolationMatrixLeftHemi)
{
    //qDebug()<<"MneDataTreeItem::onNewInterpolationMatrixLeftAvailable";
    if(m_pInterpolationItemLeftGPU) {
        m_pInterpolationItemLeftGPU->setInterpolationMatrix(pMatInterpolationMatrixLeftHemi);
    }
}

//=============================================================================================================

void MneDataTreeItem::onNewInterpolationMatrixRightAvailable(QSharedPointer<Eigen::SparseMatrix<float> > pMatInterpolationMatrixRightHemi)
{
    //qDebug()<<"MneDataTreeItem::onNewInterpolationMatrixRightAvailable";
    if(m_pInterpolationItemRightGPU) {
        m_pInterpolationItemRightGPU->setInterpolationMatrix(pMatInterpolationMatrixRightHemi);
    }
}

//=============================================================================================================

void MneDataTreeItem::onNewRtRawData(const Eigen::VectorXd &vecDataVectorLeftHemi,
                                         const Eigen::VectorXd &vecDataVectorRightHemi)
{
    if(m_pInterpolationItemLeftGPU) {
        m_pInterpolationItemLeftGPU->addNewRtData(vecDataVectorLeftHemi.cast<float>());
    }

    if(m_pInterpolationItemRightGPU) {
        m_pInterpolationItemRightGPU->addNewRtData(vecDataVectorRightHemi.cast<float>());
    }
}

//=============================================================================================================

void MneDataTreeItem::onColormapTypeChanged(const QVariant& sColormapType)
{
    if(sColormapType.canConvert<QString>()) {
        if(m_bUseGPU) {
            if(m_pInterpolationItemLeftGPU) {
                m_pInterpolationItemLeftGPU->setColormapType(sColormapType.toString());
            }

            if(m_pInterpolationItemRightGPU) {
                m_pInterpolationItemRightGPU->setColormapType(sColormapType.toString());
            }
        } else {
            if(m_pRtSourceDataController) {
                m_pRtSourceDataController->setColormapType(sColormapType.toString());
            }
        }
    }
}

//=============================================================================================================

void MneDataTreeItem::onTimeIntervalChanged(const QVariant& iMSec)
{
    if(iMSec.canConvert<int>()) {
        if(m_pRtSourceDataController) {
            m_pRtSourceDataController->setTimeInterval(iMSec.toInt());
        }
    }
}

//=============================================================================================================

void MneDataTreeItem::onDataThresholdChanged(const QVariant& vecThresholds)
{
    if(vecThresholds.canConvert<QVector3D>()) {
        if(m_bUseGPU) {
            if(m_pInterpolationItemLeftGPU) {
                m_pInterpolationItemLeftGPU->setThresholds(vecThresholds.value<QVector3D>());
            }

            if(m_pInterpolationItemRightGPU) {
                m_pInterpolationItemRightGPU->setThresholds(vecThresholds.value<QVector3D>());
            }
        } else {
            if(m_pRtSourceDataController) {
                m_pRtSourceDataController->setThresholds(vecThresholds.value<QVector3D>());
            }
        }
    }
}

//=============================================================================================================

void MneDataTreeItem::onVisualizationTypeChanged(const QVariant& sVisType)
{
    if(sVisType.canConvert<QString>()) {
        int iVisType = Data3DTreeModelItemRoles::InterpolationBased;

        if(sVisType.toString() == "Annotation based") {
            iVisType = Data3DTreeModelItemRoles::AnnotationBased;
        }

        if(m_pRtSourceDataController) {
            m_pRtSourceDataController->setVisualizationType(iVisType);
        }
    }
}

//=============================================================================================================

void MneDataTreeItem::onCheckStateLoopedStateChanged(const Qt::CheckState& checkState)
{
    if(m_pRtSourceDataController) {
        if(checkState == Qt::Checked) {
            m_pRtSourceDataController->setLoopState(true);
        } else if(checkState == Qt::Unchecked) {
            m_pRtSourceDataController->setLoopState(false);
        }
    }
}

//=============================================================================================================

void MneDataTreeItem::onNumberAveragesChanged(const QVariant& iNumAvr)
{
    if(iNumAvr.canConvert<int>()) {
        if(m_pRtSourceDataController) {
            m_pRtSourceDataController->setNumberAverages(iNumAvr.toInt());
        }
    }
}

//=============================================================================================================

//void MneDataTreeItem::onCancelDistanceChanged(const QVariant &dCancelDist)
//{
//    if(dCancelDist.canConvert<double>())
//    {
//        if(m_pRtSourceDataController) {
//            m_pRtSourceDataController->setCancelDistance(dCancelDist.toDouble());
//        }
//    }
//}

//=============================================================================================================

void MneDataTreeItem::onInterpolationFunctionChanged(const QVariant &sInterpolationFunction)
{
    if(sInterpolationFunction.canConvert<QString>()) {
        if(m_pRtSourceDataController) {
            m_pRtSourceDataController->setInterpolationFunction(sInterpolationFunction.toString());
        }
    }
}
