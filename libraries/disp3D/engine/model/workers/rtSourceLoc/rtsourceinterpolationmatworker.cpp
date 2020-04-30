//=============================================================================================================
/**
 * @file     rtsourceinterpolationmatworker.cpp
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     June, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lars Debor, Lorenz Esch. All rights reserved.
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
 * @brief    RtSourceInterpolationMatWorker class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtsourceinterpolationmatworker.h"

#include "../../../../helpers/geometryinfo/geometryinfo.h"
#include "../../../../helpers/interpolation/interpolation.h"
#include "../../items/common/types.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace MNELIB;
using namespace Eigen;
using namespace FSLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtSourceInterpolationMatWorker::RtSourceInterpolationMatWorker()
: m_bInterpolationInfoIsInit(false)
, m_iVisualizationType(Data3DTreeModelItemRoles::InterpolationBased)
, m_bAnnotationInfoIsInit(false)
, m_pMatInterpolationMat(QSharedPointer<SparseMatrix<float> >(new SparseMatrix<float>()))
, m_pMatAnnotationMat(QSharedPointer<SparseMatrix<float> >(new SparseMatrix<float>()))
{
    m_lInterpolationData.dCancelDistance = 0.05;
    m_lInterpolationData.interpolationFunction = DISP3DLIB::Interpolation::cubic;
    m_lInterpolationData.matDistanceMatrix = QSharedPointer<MatrixXd>(new MatrixXd());
}

//=============================================================================================================

void RtSourceInterpolationMatWorker::setInterpolationFunction(const QString &sInterpolationFunction)
{
    //Only supported in interpolation mode
    if(m_iVisualizationType != Data3DTreeModelItemRoles::InterpolationBased) {
        return;
    }

    if(sInterpolationFunction == QStringLiteral("Linear")) {
        m_lInterpolationData.interpolationFunction = Interpolation::linear;
    }
    else if(sInterpolationFunction == QStringLiteral("Square")) {
        m_lInterpolationData.interpolationFunction = Interpolation::square;
    }
    else if(sInterpolationFunction == QStringLiteral("Cubic")) {
        m_lInterpolationData.interpolationFunction = Interpolation::cubic;
    }
    else if(sInterpolationFunction == QStringLiteral("Gaussian")) {
        m_lInterpolationData.interpolationFunction = Interpolation::gaussian;
    }

    if(m_bInterpolationInfoIsInit == true){
        //recalculate Interpolation matrix parameters changed
        m_pMatInterpolationMat = Interpolation::createInterpolationMat(m_lInterpolationData.vecMappedSubset,
                                                                       m_lInterpolationData.matDistanceMatrix,
                                                                       m_lInterpolationData.interpolationFunction,
                                                                       m_lInterpolationData.dCancelDistance);

        emitMatrix();
    }
}

//=============================================================================================================

void RtSourceInterpolationMatWorker::setVisualizationType(int iVisType)
{
    if(m_iVisualizationType != iVisType) {
        m_iVisualizationType = iVisType;

        emitMatrix();
    }
}

//=============================================================================================================

void RtSourceInterpolationMatWorker::setCancelDistance(double dCancelDist)
{
    //Only supported in interpolation mode
    if(m_iVisualizationType != Data3DTreeModelItemRoles::InterpolationBased) {
        return;
    }

    m_lInterpolationData.dCancelDistance = dCancelDist;

    //recalculate everything because parameters changed
    calculateInterpolationOperator();

    emitMatrix();
}

//=============================================================================================================

void RtSourceInterpolationMatWorker::setInterpolationInfo(const Eigen::MatrixX3f &matVertices,
                                                          const QVector<QVector<int> > &vecNeighborVertices,
                                                          const QVector<int> &vecMappedSubset)
{
    if(matVertices.rows() == 0) {
        qDebug() << "RtSourceInterpolationMatWorker::setInterpolationInfo - Surface data is empty. Returning ...";
        return;
    }

    //set members
    m_lInterpolationData.matVertices = matVertices;
    m_lInterpolationData.vecNeighborVertices = vecNeighborVertices;
    m_lInterpolationData.vecMappedSubset = vecMappedSubset;

    m_bInterpolationInfoIsInit = true;

    calculateInterpolationOperator();

    if(m_iVisualizationType == Data3DTreeModelItemRoles::InterpolationBased) {
        emit newInterpolationMatrixCalculated(m_pMatInterpolationMat);
    }
}

//=============================================================================================================

void RtSourceInterpolationMatWorker::setAnnotationInfo(const Eigen::VectorXi &vecLabelIds,
                                                       const QList<FSLIB::Label> &lLabels,
                                                       const Eigen::VectorXi &vecVertNo)
{
    if(vecLabelIds.rows() == 0 || lLabels.isEmpty()) {
        qDebug() << "RtSourceInterpolationMatWorker::setAnnotationInfo - Annotation data is empty. Returning ...";
        return;
    }

    m_lInterpolationData.lLabels = lLabels;
    m_lInterpolationData.mapLabelIdSources.clear();

    //Generate fast lookup map for each source and corresponding label
    for(qint32 i = 0; i < vecVertNo.rows(); ++i) {
        m_lInterpolationData.mapLabelIdSources.insert(vecVertNo(i), vecLabelIds(vecVertNo(i)));
        m_lInterpolationData.vertNos.append(vecVertNo(i));
    }

    m_bAnnotationInfoIsInit = true;

    calculateAnnotationOperator();

    if(m_iVisualizationType == Data3DTreeModelItemRoles::AnnotationBased) {
        emit newInterpolationMatrixCalculated(m_pMatAnnotationMat);
    }
}

//=============================================================================================================

void RtSourceInterpolationMatWorker::calculateInterpolationOperator()
{
    if(!m_bInterpolationInfoIsInit) {
        qDebug() << "RtSourceInterpolationMatWorker::calculateInterpolationOperator - Set interpolation info first.";
        return;
    }

    //SCDC with cancel distance
    m_lInterpolationData.matDistanceMatrix = GeometryInfo::scdc(m_lInterpolationData.matVertices,
                                                                m_lInterpolationData.vecNeighborVertices,
                                                                m_lInterpolationData.vecMappedSubset,
                                                                m_lInterpolationData.dCancelDistance);

    //create Interpolation matrix
    m_pMatInterpolationMat = Interpolation::createInterpolationMat(m_lInterpolationData.vecMappedSubset,
                                                                   m_lInterpolationData.matDistanceMatrix,
                                                                   m_lInterpolationData.interpolationFunction,
                                                                   m_lInterpolationData.dCancelDistance);
    }

//=============================================================================================================

void RtSourceInterpolationMatWorker::calculateAnnotationOperator()
{
    if(!m_bAnnotationInfoIsInit) {
        qDebug() << "RtSourceInterpolationMatWorker::calculateAnnotationOperator - Set annotation info first.";
        return;
    }

    int iNumVert = 0;
    for(int i = 0; i < m_lInterpolationData.lLabels.size(); ++i) {
        iNumVert += m_lInterpolationData.lLabels.at(i).vertices.rows();
    }

    m_pMatAnnotationMat = QSharedPointer<Eigen::SparseMatrix<float> >(new Eigen::SparseMatrix<float>(iNumVert, m_lInterpolationData.vecMappedSubset.size()));

    //Color all labels respectivley to their activation
    for(int i = 0; i < m_lInterpolationData.lLabels.size(); ++i) {
        FSLIB::Label label = m_lInterpolationData.lLabels.at(i);
        QList<qint32> listSourcesVertNoLabel = m_lInterpolationData.mapLabelIdSources.keys(label.label_id);

        for(int j = 0; j < label.vertices.rows(); ++j) {
            for(int k = 0; k < listSourcesVertNoLabel.size(); ++k) {
                int colIdx = m_lInterpolationData.vertNos.indexOf(listSourcesVertNoLabel.at(k));
                m_pMatAnnotationMat->coeffRef(label.vertices(j),colIdx) = 1.0f/listSourcesVertNoLabel.size();
            }
        }
    }
}

//=============================================================================================================

void RtSourceInterpolationMatWorker::emitMatrix()
{
    switch (m_iVisualizationType) {
        case Data3DTreeModelItemRoles::InterpolationBased:
            if(!m_pMatInterpolationMat) {
                return;
            }

            if(m_pMatInterpolationMat->size() == 0) {
                calculateInterpolationOperator();
            }

            emit newInterpolationMatrixCalculated(m_pMatInterpolationMat);
            break;

        case Data3DTreeModelItemRoles::AnnotationBased:
            if(!m_pMatAnnotationMat) {
                return;
            }

            if(m_pMatAnnotationMat->size() == 0) {
                qDebug() << "RtSourceInterpolationMatWorker::setVisualizationType size == 0 Calculating";
                calculateAnnotationOperator();
            }

            emit newInterpolationMatrixCalculated(m_pMatAnnotationMat);
            break;
    }
}
