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
#include "cshdataworker.h"
#include <disp3D/engine/model/3dhelpers/custommesh.h>
#include <disp3D/helpers/geometryinfo/geometryinfo.h>
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
    : m_bIsInit(false)
    , m_pCamera(new QCamera)
    , m_pCustomMesh(new CustomMesh)
    , m_pRootEntity(new Qt3DCore::QEntity)
    , m_pComputeEntity(new Qt3DCore::QEntity(m_pRootEntity))
    , m_pMeshRenderEntity(new Qt3DCore::QEntity(m_pRootEntity))
    , m_pCamController(new Qt3DExtras::QFirstPersonCameraController(m_pRootEntity))
    , m_pComputeCommand(new QComputeCommand)
    , m_pTransform(new Qt3DCore::QTransform)
    , m_pMaterial(new ComputeMaterial)
    , m_pFramegraph(new ComputeFramegraph)
    , m_pInterpolatedSignalAttrib(new QAttribute)
    , m_fThresholdX(1e-10f)
    , m_fThresholdZ(6e-6f)
    , m_pThresholdXUniform(new QParameter(QStringLiteral("fThresholdX"), m_fThresholdX))
    , m_pThresholdZUniform(new QParameter(QStringLiteral("fThresholdZ"), m_fThresholdZ))
    , m_pRtDataWorker(new CshDataWorker)
    , m_pWeightMatBuffer(new Qt3DRender::QBuffer(Qt3DRender::QBuffer::ShaderStorageBuffer))
    , m_pInterpolatedSignalBuffer(new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer))
{
    qRegisterMetaType<Eigen::VectorXf>("Eigen::VectorXf");
    init();
}


//*************************************************************************************************************

ComputeInterpolationController::ComputeInterpolationController(const MNELIB::MNEBemSurface &tMneBemSurface,
                                                               const FIFFLIB::FiffEvoked &tEvoked,
                                                               double (*tInterpolationFunction)(double),
                                                               const qint32 tSensorType,
                                                               const double tCancelDist)
    : ComputeInterpolationController()
{
    setInterpolationData(tMneBemSurface, tEvoked, tInterpolationFunction, tSensorType, tCancelDist);
    m_bIsInit = true;
}


//*************************************************************************************************************

ComputeInterpolationController::~ComputeInterpolationController()
{
    if(m_pRtDataWorker->isRunning()) {
        m_pRtDataWorker->stop();
        delete m_pRtDataWorker;
    }
}


//*************************************************************************************************************

Qt3DCore::QEntity *ComputeInterpolationController::getRootEntity() const
{
    return m_pRootEntity.data();
}


//*************************************************************************************************************

ComputeFramegraph  *ComputeInterpolationController::getComputeFramegraph() const
{
    return m_pFramegraph.data();
}


//*************************************************************************************************************

void ComputeInterpolationController::setInterpolationData(const MNELIB::MNEBemSurface &tMneBemSurface,
                                                          const FIFFLIB::FiffEvoked &tEvoked,
                                                          double (*tInterpolationFunction)(double),
                                                          const qint32 tSensorType,
                                                          const double tCancelDist)
{
    if(m_bIsInit)
    {
        qDebug("ComputeInterpolationController::setInterpolationData: interpolation data already initialized.");
        return;
    }
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


    const uint iWeightMatRows = tMneBemSurface.rr.rows();
    const uint iWeightMatCols = m_iUsedSensors.size();

    //Create uniform for the number of columns
    m_pColsUniform = new QParameter(QStringLiteral("cols"), iWeightMatCols);
    m_pMaterial->addComputePassParameter(m_pColsUniform);

    //Create uniform for the number of rows
    m_pRowsUniform = new QParameter(QStringLiteral("rows"), iWeightMatRows);
    m_pMaterial->addComputePassParameter(m_pRowsUniform);

    //Create Weight matrix buffer and Parameter
    m_pWeightMatBuffer->setAccessType(Qt3DRender::QBuffer::Write);
    m_pWeightMatBuffer->setUsage(Qt3DRender::QBuffer::StaticDraw);
    m_pWeightMatBuffer->setData(createWeightMatBuffer(pInterpolationMatrix));
    m_pWeightMatParameter = new QParameter(QStringLiteral("WeightMat"),
                                                     QVariant::fromValue(m_pWeightMatBuffer.data()));
    m_pMaterial->addComputePassParameter(m_pWeightMatParameter);

    //Set work group size
    const uint iWorkGroupsSize = static_cast<uint>(std::ceil(std::sqrt(iWeightMatRows)));
//    m_pFramegraph->setWorkGroupSize(iWorkGroupsSize, iWorkGroupsSize ,1 );

    m_pComputeCommand->setWorkGroupX(iWorkGroupsSize);
    m_pComputeCommand->setWorkGroupY(iWorkGroupsSize);
    m_pComputeCommand->setWorkGroupZ(1);

    //Init interpolated signal buffer
    QString sInterpolatedSignalName = QStringLiteral("InterpolatedSignal");
    m_pInterpolatedSignalBuffer->setAccessType(Qt3DRender::QBuffer::ReadWrite);
    m_pInterpolatedSignalBuffer->setUsage(Qt3DRender::QBuffer::StreamCopy);
    m_pInterpolatedSignalBuffer->setData(createZeroBuffer(iWeightMatRows));

    m_pMaterial->setInterpolatedSignalBuffer(m_pInterpolatedSignalBuffer.data(), sInterpolatedSignalName);

    //Interpolated signal attribute
    m_pInterpolatedSignalAttrib->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    m_pInterpolatedSignalAttrib->setDataType(Qt3DRender::QAttribute::Float);
    m_pInterpolatedSignalAttrib->setVertexSize(1);
    m_pInterpolatedSignalAttrib->setByteOffset(0);
    m_pInterpolatedSignalAttrib->setByteStride(1 * sizeof(float));
    m_pInterpolatedSignalAttrib->setName(sInterpolatedSignalName);
    m_pInterpolatedSignalAttrib->setBuffer(m_pInterpolatedSignalBuffer);

    //Set custom mesh data
    //generate mesh base color
    MatrixX3f matVertColor = createColorMat(tMneBemSurface.rr, QColor(80, 80, 80, 255));

    //Set renderable 3D entity mesh and color data
    m_pCustomMesh->setMeshData(tMneBemSurface.rr,
                                tMneBemSurface.nn,
                                tMneBemSurface.tris,
                                matVertColor,
                                Qt3DRender::QGeometryRenderer::Triangles);
    //add interpolated signal Attribute
    m_pCustomMesh->addAttribute(m_pInterpolatedSignalAttrib);
}


//*************************************************************************************************************

void ComputeInterpolationController::addSignalData(const Eigen::MatrixXf &tSensorData)
{

    //if more data then needed is provided
    const uint iSensorSize = m_iUsedSensors.size();
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
        m_pRtDataWorker->addData(fSmallSensorData);
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
        m_pRtDataWorker->addData(fSmallSensorData);
    }
}


//*************************************************************************************************************

void ComputeInterpolationController::setNormalization(const QVector3D &tVecThresholds)
{
    m_fThresholdX = tVecThresholds.x();
    m_fThresholdZ = tVecThresholds.z();
}


//*************************************************************************************************************

void ComputeInterpolationController::startWorker()
{
    m_pRtDataWorker->start();
}


//*************************************************************************************************************

void ComputeInterpolationController::stopWorker()
{
    m_pRtDataWorker->stop();
}


//*************************************************************************************************************

void ComputeInterpolationController::setNumberAverages(const uint tNumAvr)
{
    m_pRtDataWorker->setNumberAverages(tNumAvr);
}


//*************************************************************************************************************

void ComputeInterpolationController::setInterval(const uint tMSec)
{
    m_pRtDataWorker->setInterval(tMSec);
}


//*************************************************************************************************************

void ComputeInterpolationController::setLoop(const bool tLooping)
{
    m_pRtDataWorker->setLoop(tLooping);
}


//*************************************************************************************************************

void ComputeInterpolationController::setSFreq(const double tSFreq)
{
    m_pRtDataWorker->setSFreq(tSFreq);
}


//*************************************************************************************************************

void ComputeInterpolationController::onNewRtData(const Eigen::VectorXf &tSensorData)
{
    m_pMaterial->addSignalData(tSensorData);
}


//*************************************************************************************************************

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

    //Set camera in framegraph
    m_pFramegraph->setCamera(m_pCamera);

    //Set cam controller
    m_pCamController->setCamera(m_pCamera);

    //Set thresholdX parameter
    m_pMaterial->addDrawPassParameter(m_pThresholdXUniform);
    m_pMaterial->addDrawPassParameter(m_pThresholdZUniform);

    connect(m_pRtDataWorker, &CshDataWorker::newRtData,
            this, &ComputeInterpolationController::onNewRtData);
}


//*************************************************************************************************************

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
            rawVertexArray[iCtr] = static_cast<float>(tInterpolationMatrix->coeff(i, j));
            iCtr++;
        }
    }

    return bufferData;
}


//*************************************************************************************************************

QByteArray ComputeInterpolationController::createZeroBuffer(const uint tBufferSize)
{
    QByteArray bufferData;
    bufferData.resize(tBufferSize * (int)sizeof(float));
    float *rawVertexArray = reinterpret_cast<float *>(bufferData.data());

    //Set default values
    for(uint i = 0; i < tBufferSize; ++i)
    {
        rawVertexArray[i] = 0.0f;
    }
    return bufferData;
}


//*************************************************************************************************************

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


//*************************************************************************************************************
