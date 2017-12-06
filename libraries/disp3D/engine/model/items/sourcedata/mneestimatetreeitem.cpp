//=============================================================================================================
/**
* @file     mneestimatetreeitem.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     December, 2016
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
* @brief    MneEstimateTreeItem class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mneestimatetreeitem.h"
#include "../../workers/rtSourceLoc/rtsourcedatacontroller.h"
#include "../common/metatreeitem.h"
#include "../common/abstractmeshtreeitem.h"
#include "../../3dhelpers/custommesh.h"

#include <mne/mne_sourceestimate.h>
#include <mne/mne_forwardsolution.h>
#include <fs/surfaceset.h>
#include <fs/annotationset.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QVector3D>
#include <Qt3DCore/QEntity>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace MNELIB;
using namespace DISP3DLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MneEstimateTreeItem::MneEstimateTreeItem(int iType, const QString &text)
: AbstractTreeItem(iType, text)
, m_bIsDataInit(false)
{
    initItem();
}


//*************************************************************************************************************

MneEstimateTreeItem::~MneEstimateTreeItem()
{
    m_pRtSourceDataController->deleteLater();
}


//*************************************************************************************************************

void MneEstimateTreeItem::initItem()
{
    this->setEditable(false);
    this->setToolTip("MNE Estimate item");

    //Add items
    QList<QStandardItem*> list;
    QVariant data;

    MetaTreeItem* pItemStreamStatus = new MetaTreeItem(MetaTreeItemTypes::StreamStatus, "Stream data on/off");
    connect(pItemStreamStatus, &MetaTreeItem::checkStateChanged,
            this, &MneEstimateTreeItem::onCheckStateWorkerChanged);
    list << pItemStreamStatus;
    list << new QStandardItem(pItemStreamStatus->toolTip());
    this->appendRow(list);
    pItemStreamStatus->setCheckable(true);
    pItemStreamStatus->setCheckState(Qt::Unchecked);

    data.setValue(false);
    pItemStreamStatus->setData(data, MetaTreeItemRoles::StreamStatus);

    MetaTreeItem* pItemVisuaizationType = new MetaTreeItem(MetaTreeItemTypes::VisualizationType, "Vertex based");
    connect(pItemVisuaizationType, &MetaTreeItem::dataChanged,
            this, &MneEstimateTreeItem::onVisualizationTypeChanged);
    list.clear();
    list << pItemVisuaizationType;
    list << new QStandardItem(pItemVisuaizationType->toolTip());
    this->appendRow(list);
    data.setValue(QString("Vertex based"));
    pItemVisuaizationType->setData(data, MetaTreeItemRoles::VisualizationType);

    MetaTreeItem* pItemColormapType = new MetaTreeItem(MetaTreeItemTypes::ColormapType, "Hot");
    connect(pItemColormapType, &MetaTreeItem::dataChanged,
            this, &MneEstimateTreeItem::onColormapTypeChanged);
    list.clear();
    list << pItemColormapType;
    list << new QStandardItem(pItemColormapType->toolTip());
    this->appendRow(list);
    data.setValue(QString("Hot"));
    pItemColormapType->setData(data, MetaTreeItemRoles::ColormapType);

    MetaTreeItem* pItemSourceLocNormValue = new MetaTreeItem(MetaTreeItemTypes::DataThreshold, "0.0,5.5,15");
    connect(pItemSourceLocNormValue, &MetaTreeItem::dataChanged,
            this, &MneEstimateTreeItem::onDataThresholdChanged);
    list.clear();
    list << pItemSourceLocNormValue;
    list << new QStandardItem(pItemSourceLocNormValue->toolTip());
    this->appendRow(list);
    data.setValue(QVector3D(0.0,5.5,15));
    pItemSourceLocNormValue->setData(data, MetaTreeItemRoles::DataThreshold);

    MetaTreeItem *pItemStreamingInterval = new MetaTreeItem(MetaTreeItemTypes::StreamingTimeInterval, "17");
    connect(pItemStreamingInterval, &MetaTreeItem::dataChanged,
            this, &MneEstimateTreeItem::onTimeIntervalChanged);
    list.clear();
    list << pItemStreamingInterval;
    list << new QStandardItem(pItemStreamingInterval->toolTip());
    this->appendRow(list);
    data.setValue(17);
    pItemStreamingInterval->setData(data, MetaTreeItemRoles::StreamingTimeInterval);

    MetaTreeItem *pItemLoopedStreaming = new MetaTreeItem(MetaTreeItemTypes::LoopedStreaming, "Looping on/off");
    connect(pItemLoopedStreaming, &MetaTreeItem::checkStateChanged,
            this, &MneEstimateTreeItem::onCheckStateLoopedStateChanged);
    pItemLoopedStreaming->setCheckable(true);
    pItemLoopedStreaming->setCheckState(Qt::Checked);
    list.clear();
    list << pItemLoopedStreaming;
    list << new QStandardItem(pItemLoopedStreaming->toolTip());
    this->appendRow(list);

    MetaTreeItem *pItemAveragedStreaming = new MetaTreeItem(MetaTreeItemTypes::NumberAverages, "1");
    connect(pItemAveragedStreaming, &MetaTreeItem::dataChanged,
            this, &MneEstimateTreeItem::onNumberAveragesChanged);
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
//            this, &MneEstimateTreeItem::onCancelDistanceChanged);

    MetaTreeItem* pInterpolationFunction = new MetaTreeItem(MetaTreeItemTypes::InterpolationFunction, "Cubic");
    list.clear();
    list << pInterpolationFunction;
    list << new QStandardItem(pInterpolationFunction->toolTip());
    this->appendRow(list);
    data.setValue(QString("Cubic"));
    pInterpolationFunction->setData(data, MetaTreeItemRoles::InterpolationFunction);
    connect(pInterpolationFunction, &MetaTreeItem::dataChanged,
            this, &MneEstimateTreeItem::onInterpolationFunctionChanged);
}


//*************************************************************************************************************

void MneEstimateTreeItem::initData(const MNEForwardSolution& tForwardSolution,
                                   const SurfaceSet& tSurfSet,
                                   const AnnotationSet& tAnnotSet,
                                   Qt3DCore::QEntity* p3DEntityParent)
{   
    if(tForwardSolution.src.size() < 2) {
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

    //Process annotation data
//    MatrixX3f matAnnotColors(tAnnotation.getVertices().rows(), 3);

//    QList<FSLIB::Label> qListLabels;
//    QList<RowVector4i> qListLabelRGBAs;

//    tAnnotation.toLabels(tSurface, qListLabels, qListLabelRGBAs);

//    for(int i = 0; i < qListLabels.size(); ++i) {
//        FSLIB::Label label = qListLabels.at(i);
//        for(int j = 0; j<label.vertices.rows(); j++) {
//            QColor patchColor;
//            patchColor.setRed(qListLabelRGBAs.at(i)(0));
//            patchColor.setGreen(qListLabelRGBAs.at(i)(1));
//            patchColor.setBlue(qListLabelRGBAs.at(i)(2));

//            patchColor = patchColor.darker(200);

//            if(label.vertices(j) < matAnnotColors.rows()) {
//                matAnnotColors(label.vertices(j),0) = patchColor.redF();
//                matAnnotColors(label.vertices(j),1) = patchColor.greenF();
//                matAnnotColors(label.vertices(j),2) = patchColor.blueF();
//            }
//        }
//    }
//    vecLabelIdsLeftHemi = tAnnotation.getLabelIds();
//    vecLabelIdsRightHemi = tAnnotation.getLabelIds();

    //Create CpuInterpolationItems
    if(!m_pInterpolationItemLeft)
    {
        m_pInterpolationItemLeft = new AbstractMeshTreeItem(p3DEntityParent,
                                                        Data3DTreeModelItemTypes::AbstractMeshItem,
                                                        QStringLiteral("3D Plot - Left"));

        //Create color from curvature information with default gyri and sulcus colors
        MatrixX3f matVertColor = AbstractMeshTreeItem::createVertColor(tSurfSet[0].rr().rows());

        m_pInterpolationItemLeft->getCustomMesh()->setMeshData(tSurfSet[0].rr(),
                                                               tSurfSet[0].nn(),
                                                               tSurfSet[0].tris(),
                                                               matVertColor,
                                                               Qt3DRender::QGeometryRenderer::Triangles);

        QList<QStandardItem*> list;
        list << m_pInterpolationItemLeft;
        list << new QStandardItem(m_pInterpolationItemLeft->toolTip());
        this->appendRow(list);
    }

    if(!m_pInterpolationItemRight)
    {
        m_pInterpolationItemRight = new AbstractMeshTreeItem(p3DEntityParent,
                                                        Data3DTreeModelItemTypes::AbstractMeshItem,
                                                        QStringLiteral("3D Plot - Right"));

        //Create color from curvature information with default gyri and sulcus colors
        MatrixX3f matVertColor = AbstractMeshTreeItem::createVertColor(tSurfSet[1].rr().rows());

        m_pInterpolationItemRight->getCustomMesh()->setMeshData(tSurfSet[1].rr(),
                                                                tSurfSet[1].nn(),
                                                                tSurfSet[1].tris(),
                                                                matVertColor,
                                                                Qt3DRender::QGeometryRenderer::Triangles);

        QList<QStandardItem*> list;
        list << m_pInterpolationItemRight;
        list << new QStandardItem(m_pInterpolationItemRight->toolTip());
        this->appendRow(list);
    }


    //set rt data corresponding to the hemisphere
    if(!m_pRtSourceDataController) {
        m_pRtSourceDataController = new RtSourceDataController();
    }

    connect(m_pRtSourceDataController.data(), &RtSourceDataController::newRtSmoothedDataAvailable,
            this, &MneEstimateTreeItem::onNewRtSmoothedDataAvailable);

    m_pRtSourceDataController->setInterpolationInfo(tForwardSolution.src[0].rr,
                                                    tForwardSolution.src[1].rr,
                                                    tForwardSolution.src[0].neighbor_vert,
                                                    tForwardSolution.src[1].neighbor_vert,
                                                    clustVertNoLeft,
                                                    clustVertNoRight);

//    m_pRtSourceDataController->setAnnotationData(vecLabelIdsLeftHemi,
//                                                vecLabelIdsRightHemi,
//                                                lLabelsLeftHemi,
//                                                lLabelsRightHemi);

    m_bIsDataInit = true;
}


//*************************************************************************************************************

void MneEstimateTreeItem::addData(const MNESourceEstimate& tSourceEstimate)
{
    if(!m_bIsDataInit) {
        qDebug() << "MneEstimateTreeItem::addData - Rt source loc item has not been initialized yet!";
        return;
    }

    //Set new data into item's data. The set data is for example needed in the delegate to calculate the histogram.
    QVariant data;
    data.setValue(tSourceEstimate.data);
    this->setData(data, Data3DTreeModelItemRoles::Data);

    if(m_pRtSourceDataController) {
        m_pRtSourceDataController->addData(tSourceEstimate.data);
    }
}


//*************************************************************************************************************

void MneEstimateTreeItem::setLoopState(bool state)
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


//*************************************************************************************************************

void MneEstimateTreeItem::setStreamingState(bool state)
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


//*************************************************************************************************************

void MneEstimateTreeItem::setTimeInterval(int iMSec)
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


//*************************************************************************************************************

void MneEstimateTreeItem::setNumberAverages(int iNumberAverages)
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


//*************************************************************************************************************

void MneEstimateTreeItem::setColormapType(const QString& sColormap)
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


//*************************************************************************************************************

void MneEstimateTreeItem::setVisualizationType(const QString& sVisualizationType)
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


//*************************************************************************************************************

void MneEstimateTreeItem::setThresholds(const QVector3D& vecThresholds)
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


//*************************************************************************************************************

//void MneEstimateTreeItem::setCancelDistance(double dCancelDist)
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


//*************************************************************************************************************

void MneEstimateTreeItem::setInterpolationFunction(const QString &sInterpolationFunction)
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


//*************************************************************************************************************

void MneEstimateTreeItem::setSFreq(const double dSFreq)
{
    if(m_pRtSourceDataController) {
        m_pRtSourceDataController->setSFreq(dSFreq);
    }
}


//*************************************************************************************************************

void MneEstimateTreeItem::onCheckStateWorkerChanged(const Qt::CheckState& checkState)
{
    if(m_pRtSourceDataController) {
        if(checkState == Qt::Checked) {
            m_pRtSourceDataController->setStreamingState(true);
        } else if(checkState == Qt::Unchecked) {
            m_pRtSourceDataController->setStreamingState(false);
        }
    }
}


//*************************************************************************************************************

void MneEstimateTreeItem::onNewRtSmoothedDataAvailable(const Eigen::MatrixX3f &matColorMatrixLeftHemi,
                                                       const Eigen::MatrixX3f &matColorMatrixRightHemi)
{    
    QVariant data;

    if(m_pInterpolationItemLeft)
    {
        data.setValue(matColorMatrixLeftHemi);
        m_pInterpolationItemLeft->setVertColor(data);
    }

    if(m_pInterpolationItemRight)
    {
        data.setValue(matColorMatrixRightHemi);
        m_pInterpolationItemRight->setVertColor(data);
    }
}


//*************************************************************************************************************

void MneEstimateTreeItem::onColormapTypeChanged(const QVariant& sColormapType)
{
    if(sColormapType.canConvert<QString>()) {
        if(m_pRtSourceDataController) {
            m_pRtSourceDataController->setColormapType(sColormapType.toString());
        }
    }
}


//*************************************************************************************************************

void MneEstimateTreeItem::onTimeIntervalChanged(const QVariant& iMSec)
{
    if(iMSec.canConvert<int>()) {
        if(m_pRtSourceDataController) {
            m_pRtSourceDataController->setTimeInterval(iMSec.toInt());
        }
    }
}


//*************************************************************************************************************

void MneEstimateTreeItem::onDataThresholdChanged(const QVariant& vecThresholds)
{
    if(vecThresholds.canConvert<QVector3D>()) {
        if(m_pRtSourceDataController) {
            m_pRtSourceDataController->setThresholds(vecThresholds.value<QVector3D>());
        }
    }
}


//*************************************************************************************************************

void MneEstimateTreeItem::onVisualizationTypeChanged(const QVariant& sVisType)
{
    if(sVisType.canConvert<QString>()) {
        int iVisType = Data3DTreeModelItemRoles::VertexBased;

        if(sVisType.toString() == "Annotation based") {
            iVisType = Data3DTreeModelItemRoles::AnnotationBased;
        }

        if(sVisType.toString() == "Smoothing based") {
            iVisType = Data3DTreeModelItemRoles::SmoothingBased;
        }

        if(m_pRtSourceDataController) {
            //m_pRtSourceDataController->setVisualizationType(iVisType);
        }
    }
}


//*************************************************************************************************************

void MneEstimateTreeItem::onCheckStateLoopedStateChanged(const Qt::CheckState& checkState)
{
    if(m_pRtSourceDataController) {
        if(checkState == Qt::Checked) {
            m_pRtSourceDataController->setLoopState(true);
        } else if(checkState == Qt::Unchecked) {
            m_pRtSourceDataController->setLoopState(false);
        }
    }
}


//*************************************************************************************************************

void MneEstimateTreeItem::onNumberAveragesChanged(const QVariant& iNumAvr)
{
    if(iNumAvr.canConvert<int>()) {
        if(m_pRtSourceDataController) {
            m_pRtSourceDataController->setNumberAverages(iNumAvr.toInt());
        }
    }
}


//*************************************************************************************************************

//void MneEstimateTreeItem::onCancelDistanceChanged(const QVariant &dCancelDist)
//{
//    if(dCancelDist.canConvert<double>())
//    {
//        if(m_pRtSourceDataController) {
//            m_pRtSourceDataController->setCancelDistance(dCancelDist.toDouble());
//        }
//    }
//}


//*************************************************************************************************************

void MneEstimateTreeItem::onInterpolationFunctionChanged(const QVariant &sInterpolationFunction)
{
    if(sInterpolationFunction.canConvert<QString>())
    {
        if(m_pRtSourceDataController) {
            m_pRtSourceDataController->setInterpolationFunction(sInterpolationFunction.toString());
        }
    }
}
