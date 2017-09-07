//=============================================================================================================
/**
* @file     computeinterpolationcontroller.cpp
* @author   Lars Debor <lars.debor@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     August, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lars Debor and Matti Hamalainen. All rights reserved.
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
* @brief    ComputeInterpolationController class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "computeinterpolationcontroller.h"
#include "computeframegraph.h"
#include "computematerial.h"
#include <disp3D/engine/model/3dhelpers/custommesh.h>
#include <geometryInfo/geometryinfo.h>
#include <fiff/fiff_evoked.h>
#include <fiff/fiff_types.h>
#include <mne/mne_bem_surface.h>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <Qt3DExtras/QFirstPersonCameraController>
#include <Qt3DCore/QTransform>
#include <Qt3DCore/QEntity>
#include <Qt3DRender/QComputeCommand>
#include <Qt3DRender/QCamera>
#include <Qt3DRender/QBuffer>
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QParameter>

//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CSH;
using namespace Qt3DRender;
using namespace DISP3DLIB;
using namespace INTERPOLATION;
using namespace GEOMETRYINFO;
using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ComputeInterpolationController::ComputeInterpolationController()
    : m_pCamera(new QCamera)
    , m_pCustomMesh(new CustomMesh)
    , m_pRootEntity(new Qt3DCore::QEntity)
    , m_pComputeEntity(new Qt3DCore::QEntity(m_pRootEntity))
    , m_pMeshRenderEntity(new Qt3DCore::QEntity(m_pRootEntity))
    , m_pCamController(new Qt3DExtras::QFirstPersonCameraController(m_pRootEntity))
    , m_pComputeCommand(new QComputeCommand)
    , m_pTransform(new Qt3DCore::QTransform)
    , m_pMaterial(new ComputeMaterial)
    , m_pFramegraph(new ComputeFramegraph)
{
    init();
}

QPointer<Qt3DCore::QEntity> ComputeInterpolationController::getRootEntity() const
{
    return m_pRootEntity;
}

QPointer<ComputeFramegraph> ComputeInterpolationController::getComputeFramegraph() const
{
    return m_pFramegraph;
}

void ComputeInterpolationController::setInterpolationData(const MNELIB::MNEBemSurface &tMneBemSurface,
                                                          const FIFFLIB::FiffEvoked &tEvoked,
                                                          double (*tInterpolationFunction)(double),
                                                          const qint32 tSensorType,
                                                          const double tCancelDist
                                                          )
{
    //fill QVector with the right sensor positions
    QVector<Eigen::Vector3f> vecSensorPos;
    m_iUsedSensors.clear();
    uint iCounter = 0;
    for(const FIFFLIB::FiffChInfo &info : tEvoked.info.chs) {
        //Only take EEG with V as unit or MEG magnetometers with T as unit
        if(info.kind == tSensorType && (info.unit == FIFF_UNIT_T || info.unit == FIFF_UNIT_V)) {
            vecSensorPos.push_back(info.chpos.r0);

            //save the number of the sensor
            m_iUsedSensors.push_back(iCounter);
        }
        iCounter++;
    }

    //Create bad channel idx list
    for(const QString &bad : tEvoked.info.bads) {
        m_iSensorsBad.push_back(tEvoked.info.ch_names.indexOf(bad));
    }

    //sensor projecting
    QSharedPointer<QVector<qint32>> pMappedSubSet = GeometryInfo::projectSensors(tMneBemSurface, vecSensorPos);

    //SCDC with cancel distance
    QSharedPointer<MatrixXd> pDistanceMatrix = GeometryInfo::scdc(tMneBemSurface, pMappedSubSet, tCancelDist);

    //filtering of bad channels out of the distance table
    GeometryInfo::filterBadChannels(pDistanceMatrix, tEvoked.info, tSensorType);

    //create weight matrix
    QSharedPointer<SparseMatrix<double>> pInterpolationMatrix = Interpolation::createInterpolationMat(pMappedSubSet,
                                                                               pDistanceMatrix,
                                                                               tInterpolationFunction,
                                                                               tCancelDist,
                                                                               tEvoked.info,
                                                                               tSensorType);


    //create weight matrix Uniforms
    const uint iWeightMatRows = tMneBemSurface.rr.rows();
    const uint iWeightMatCols = m_iUsedSensors.size();
    QParameter *pRowUniform = new QParameter(QStringLiteral("rows"), iWeightMatRows);
    m_pMaterial->addComputePassParameter(pRowUniform);
    m_pParameters.insert(pRowUniform->name(), pRowUniform);

    QParameter *pColsUniform = new QParameter(QStringLiteral("cols"), iWeightMatCols);
    m_pMaterial->addComputePassParameter(pColsUniform);
    m_pParameters.insert(pColsUniform->name(), pColsUniform);

    //Create Weight matrix buffer and Parameter
    Qt3DRender::QBuffer *pWeightMatBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::ShaderStorageBuffer);
    pWeightMatBuffer->setData(createWeightMatBuffer(pInterpolationMatrix));
    QParameter *pWeightMatParameter = new QParameter(QStringLiteral("WeightMat"),
                                                     QVariant::fromValue(pWeightMatBuffer));
    m_pMaterial->addComputePassParameter(pWeightMatParameter);
    m_pBuffers.insert(pWeightMatParameter->name(), pWeightMatBuffer);
    m_pParameters.insert(pWeightMatParameter->name(), pWeightMatParameter);


    //Set work group size
    m_pFramegraph->setWorkGroupSize(iWeightMatRows, 0 ,0 );

    //Init interpolated signal buffer
    QString sInterpolatedSignalName = QStringLiteral("InterpolatedSignal");
    Qt3DRender::QBuffer *pInterpolatedSignalBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer);
    pInterpolatedSignalBuffer->setData(createZeroBuffer( 3 * iWeightMatRows));
    m_pBuffers.insert(sInterpolatedSignalName, pInterpolatedSignalBuffer);

    //@TODO setYoutBuffer umschreiben
    m_pMaterial->setYOutBuffer(pInterpolatedSignalBuffer);

    //Interpolated signal attribute
    QAttribute *m_pInterpolatedSignalAttrib = new QAttribute();
    m_pInterpolatedSignalAttrib->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    m_pInterpolatedSignalAttrib->setDataType(Qt3DRender::QAttribute::Float);
    m_pInterpolatedSignalAttrib->setVertexSize(1);
    m_pInterpolatedSignalAttrib->setByteOffset(0);
    m_pInterpolatedSignalAttrib->setByteStride(1 * sizeof(float));
    m_pInterpolatedSignalAttrib->setName(sInterpolatedSignalName);
    m_pInterpolatedSignalAttrib->setBuffer(pInterpolatedSignalBuffer);

    //Set custom mesh data
    //generate base color
    MatrixX3f matVertColor = createColorMat(tMneBemSurface.rr, QColor(0,0,255,255));

    //Set renderable 3D entity mesh and color data
    m_pCustomMesh->setMeshData(tMneBemSurface.rr,
                                tMneBemSurface.nn,
                                tMneBemSurface.tris,
                                matVertColor,
                                Qt3DRender::QGeometryRenderer::Triangles);
    //add interpolated signal Attribute
    m_pCustomMesh->addAttrib(m_pInterpolatedSignalAttrib);



}

void ComputeInterpolationController::addSignalData(const Eigen::MatrixXf &tSensorData)
{
    ///////////////////////////////////
    //if more data then needed is provided
    const int iSensorSize = m_iUsedSensors.size();
    if(tSensorData.rows() > iSensorSize)
    {
        MatrixXf fSmallSensorData(iSensorSize, tSensorData.cols());
        for(uint i = 0 ; i < iSensorSize; ++i)
        {
            //Set bad channels to zero
            if(m_iSensorsBad.contains(m_iUsedSensors[i])) {
                fSmallSensorData.row(i).setZero();
            } else {
                fSmallSensorData.row(i) = tSensorData.row(m_iUsedSensors[i]);
            }
        }
        m_pMaterial->addSignalData(fSmallSensorData);
    }
    else
    {
        //Set bad channels to zero
        MatrixXf fSmallSensorData = tSensorData;
        for(uint i = 0 ; i < fSmallSensorData.rows(); ++i)
        {
            if(m_iSensorsBad.contains(m_iUsedSensors[i])) {
                fSmallSensorData.row(i).setZero();
            }
        }
        m_pMaterial->addSignalData(fSmallSensorData);
    }
}

void ComputeInterpolationController::init()
{  
    //Set Transform
    m_pTransform->setScale3D(QVector3D(50.0, 50.0, 50.0));

    //Set compute enity
    m_pComputeEntity->addComponent(m_pComputeCommand);
    m_pComputeEntity->addComponent(m_pMaterial);

    //Set mesh render enity
    m_pMeshRenderEntity->addComponent(m_pCustomMesh);
    m_pMeshRenderEntity->addComponent(m_pMaterial);
    m_pMeshRenderEntity->addComponent(m_pTransform);
    
    //Set camera
    m_pCamera->setProjectionType(QCameraLens::PerspectiveProjection);
    m_pCamera->setViewCenter(QVector3D(0, 0, 0));
    m_pCamera->setPosition(QVector3D(0, 0, 40.0));
    m_pCamera->setNearPlane(0.1f);
    m_pCamera->setFarPlane(1000.0f);
    m_pCamera->setFieldOfView(25.0f);
    m_pCamera->setAspectRatio(1.33f);

    //Set cam controller
    m_pCamController->setCamera(m_pCamera);

    
}

QByteArray ComputeInterpolationController::createWeightMatBuffer(QSharedPointer<Eigen::SparseMatrix<double> > tInterpolationMatrix)
{
    QByteArray bufferData;

    const uint iRows = tInterpolationMatrix->rows();
    const uint iCols = tInterpolationMatrix->cols();

    bufferData.resize(iRows * iCols * (int)sizeof(float));
    float *rawVertexArray = reinterpret_cast<float *>(bufferData.data());

    unsigned int iCtr = 0;
    for(uint i = 0; i < iRows; ++i)
    {
        for(uint j = 0; j < iCols; ++j)
        {
            //@TODO this is probably not the best way to extract the weight matrix components
            rawVertexArray[iCtr] = tInterpolationMatrix->coeff(i, j);
            iCtr++;
        }
    }

    return bufferData;
}

QByteArray ComputeInterpolationController::createZeroBuffer(const uint tBufferSize)
{
    QByteArray bufferData;
    bufferData.resize(tBufferSize * (int)sizeof(float));
    float *rawVertexArray = reinterpret_cast<float *>(bufferData.data());

    //Set default values
    for(int i = 0; i < tBufferSize; ++i)
    {
        rawVertexArray[i] = 0.0f;
    }
    return bufferData;
}

MatrixX3f ComputeInterpolationController::createColorMat(const MatrixXf &tVertices, const QColor &tColor)
{
    MatrixX3f matColor(tVertices.rows(),3);

    for(int i = 0; i < matColor.rows(); ++i) {
        matColor(i,0) = tColor.redF();
        matColor(i,1) = tColor.greenF();
        matColor(i,2) = tColor.blueF();
    }

    return matColor;
}

/* YOut buffer hängt ab vom Surface
 *
 * Uniform rows  und cols hängt ab von weight matrix #
 *
 * weight matrix buffer hängt ab von interpolation matrix #
 *
 * addSignal größe auf eeg bzw. meg anpassen
 *
 * sensoren speichern #
 *
 * workgroupsize setzen abhängig von weight matrix
 *
 *
 *
*/
//*************************************************************************************************************
