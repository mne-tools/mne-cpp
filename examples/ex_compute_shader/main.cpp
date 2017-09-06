//=============================================================================================================
/**
* @file     main.cpp
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
* @brief    Example of using compute shaders for interpolation
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================


#include <mne/mne_sourceestimate.h>
#include <mne/mne_bem.h>
#include <disp3D/engine/model/3dhelpers/custommesh.h>
#include <computeShader/computematerial.h>
#include <computeShader/computeframegraph.h>

#include <iostream>

#include <geometryInfo/geometryinfo.h>
#include <interpolation/interpolation.h>
#include <fiff/fiff_constants.h>

#include <cmath>
#include <cstdlib>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//@TODO check for unnecessary includes
#include <QApplication>
#include <QMainWindow>
#include <QCommandLineParser>
#include <QDateTime>
#include <QFile>

#include <QByteArray>
#include <QRectF>
#include <Qt3DCore>
#include <Qt3DRender>
#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DExtras/QCylinderMesh>
#include <Qt3DExtras/QSphereGeometry>
#include <Qt3DExtras/QFirstPersonCameraController>
#include <Qt3DExtras/QTorusMesh>
#include <Qt3DExtras>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace FSLIB;
using namespace FIFFLIB;
using namespace GEOMETRYINFO;
using namespace INTERPOLATION;
using namespace CSH;
using namespace Qt3DRender;


//*************************************************************************************************************
//=============================================================================================================
// DECLARED FUNCTIONS
//=============================================================================================================

QByteArray createVertexBufferFromBemSurface(const MNEBemSurface &tBemSurface);
QByteArray createColorBuffer(const int iVertNum);
MatrixX3f createVertColor(const MatrixXf& vertices, const QColor& color);
QByteArray createWeightMatBuffer(QSharedPointer<SparseMatrix<double> > tInterpolationMatrix);


//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================


//=============================================================================================================
/**
* The function main marks the entry point of the program.
* By default, main has the storage class extern.
*
* @param [in] argc (argument count) is an integer that indicates how many arguments were entered on the command line when the program was started.
* @param [in] argv (argument vector) is an array of pointers to arrays of character objects. The array objects are null-terminated strings, representing the arguments that were entered on the command line when the program was started.
* @return the value that was set to exit() (which is 0 if exit() is called via quit()).
*/
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Command Line Parser
    QCommandLineParser parser;
    parser.setApplicationDescription("ex_interpolation");
    parser.addHelpOption();

    QCommandLineOption subjectPathOption("subjectPath", "Selected subject path <subjectPath>.", "subjectPath", "./MNE-sample-data/subjects");
    QCommandLineOption surfOption("surfType", "Surface type <type>.", "type", "pial");
    QCommandLineOption annotOption("annotType", "Annotation type <type>.", "type", "aparc.a2009s");
    QCommandLineOption hemiOption("hemi", "Selected hemisphere <hemi>.", "hemi", "2");
    QCommandLineOption subjectOption("subject", "Selected subject <subject>.", "subject", "sample");
    QCommandLineOption sampleEvokedFileOption("ave", "Path to the evoked/average <file>.", "file", "./MNE-sample-data/MEG/sample/sample_audvis-ave.fif");
    QCommandLineOption evokedIndexOption("aveIdx", "The average <index> to choose from the average file.", "index", "3");

    parser.addOption(surfOption);
    parser.addOption(annotOption);
    parser.addOption(hemiOption);
    parser.addOption(subjectOption);
    parser.addOption(subjectPathOption);
    parser.addOption(sampleEvokedFileOption);
    parser.addOption(evokedIndexOption);

    parser.process(a);

    //acquire sensor positions
    QFile t_fileEvoked(parser.value(sampleEvokedFileOption));


    // Load data
    QPair<QVariant, QVariant> baseline(QVariant(), 0);
    FiffEvoked evoked(t_fileEvoked, parser.value(evokedIndexOption).toInt(), baseline);
    if(evoked.isEmpty())
    {
        return 1;
    }

    // positions of EEG and MEG sensors
    QVector<Vector3f> eegSensors;
    QVector<Vector3f> megSensors;
    //fill both QVectors with the right sensor positions
    for( const FiffChInfo &info : evoked.info.chs)
    {
        //EEG
        if(info.kind == FIFFV_EEG_CH && info.unit == FIFF_UNIT_V)
        {
            eegSensors.push_back(info.chpos.r0);
        }
        //MEG
        if(info.kind == FIFFV_MEG_CH && info.unit == FIFF_UNIT_T)
        {
            megSensors.push_back(info.chpos.r0);
        }
    }

    //acquire surface data
    //QFile t_filesensorSurfaceVV("./MNE-sample-data/subjects/sample/bem/sample-head.fif");
    QFile t_filesensorSurfaceVV("./MNE-sample-data/subjects/sample/bem/sample-5120-5120-5120-bem.fif");
    MNEBem t_sensorSurfaceVV(t_filesensorSurfaceVV);

    //projecting with EEG
    qint64 startTimeProjecting = QDateTime::currentMSecsSinceEpoch();
    QSharedPointer<QVector<qint32>> mappedSubSet = GeometryInfo::projectSensors(t_sensorSurfaceVV[0], eegSensors);
    std::cout <<  "Projecting duration: " << QDateTime::currentMSecsSinceEpoch() - startTimeProjecting <<" ms " << std::endl;

    //SCDC with cancel distance 0.03
    qint64 startTimeScdc = QDateTime::currentMSecsSinceEpoch();
    QSharedPointer<MatrixXd> distanceMatrix = GeometryInfo::scdc(t_sensorSurfaceVV[0], mappedSubSet, 0.03);
    std::cout << "SCDC duration: " << QDateTime::currentMSecsSinceEpoch() - startTimeScdc<< " ms " << std::endl;

    //filter out bad EEG channels
    GeometryInfo::filterBadChannels(distanceMatrix, evoked.info, FIFFV_EEG_CH);

    //weight matrix
    qint64 startTimeWMat = QDateTime::currentMSecsSinceEpoch();
    QSharedPointer<SparseMatrix<double> > interpolationMatrix = Interpolation::createInterpolationMat(mappedSubSet, distanceMatrix, Interpolation::linear, DOUBLE_INFINITY, evoked.info, FIFFV_EEG_CH);
    std::cout << "Weight matrix duration: " << QDateTime::currentMSecsSinceEpoch() - startTimeWMat<< " ms " << std::endl;

    //realtime interpolation (1 iteration)
    VectorXd signal = VectorXd::Random(eegSensors.size());
    qint64 startTimeRTI = QDateTime::currentMSecsSinceEpoch();
    Interpolation::interpolateSignal(interpolationMatrix, signal);
    std::cout << "Real time interpol. : " << QDateTime::currentMSecsSinceEpoch() - startTimeRTI << " ms " << std::endl;


    //########################################################################################
    //
    // QT3D INIT
    //
    //########################################################################################

    Qt3DExtras::Qt3DWindow view;

    Qt3DCore::QEntity *rootEntity = new Qt3DCore::QEntity();

    Qt3DRender::QBuffer *pYOutBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer);
    pYOutBuffer->setData(createColorBuffer(t_sensorSurfaceVV[0].rr.rows()));

    //Color buffer size 2562
    std::cout << "Color buffer size: " << t_sensorSurfaceVV[0].rr.rows() << std::endl;
    std::cout << "EEG senors size: " << eegSensors.size() << std::endl;

    ComputeMaterial *pComputeMaterial = new ComputeMaterial();
    pComputeMaterial->setYOutBuffer(pYOutBuffer);
    //add custom parameters to computeMaterial
    //@TODO change this values to the correct one
    const unsigned int iWeightMatRows = t_sensorSurfaceVV[0].rr.rows();
    const unsigned int iWeightMatCols = eegSensors.size();
    QParameter *pRowUniform = new QParameter(QStringLiteral("rows"), iWeightMatRows);
    pComputeMaterial->addComputePassParameter(pRowUniform);
    QParameter *pColsUniform = new QParameter(QStringLiteral("cols"), iWeightMatCols);
    pComputeMaterial->addComputePassParameter(pColsUniform);

    //Create Weight matrix buffer and Parameter
    Qt3DRender::QBuffer *pWeightMatBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::ShaderStorageBuffer);
    pWeightMatBuffer->setData(createWeightMatBuffer(interpolationMatrix));

    QParameter *pWeightMatParameter = new QParameter(QStringLiteral("WeightMat"),
                                                     QVariant::fromValue(pWeightMatBuffer));
    pComputeMaterial->addComputePassParameter(pWeightMatParameter);

    //create random EEG signal for testing
    pComputeMaterial->createSignalMatrix(eegSensors.size(), 300);






    ComputeFramegraph *pFramegraph = new ComputeFramegraph();
    //@TODO wie workgroup size bestimmen besser Aufteilung finden es gibt 2562 vertecies
    pFramegraph->setWorkGroupSize(iWeightMatRows, 0 ,0 );

    //Compute entity
    Qt3DCore::QEntity *pComputeEntity = new Qt3DCore::QEntity(rootEntity);
    QComputeCommand *pComputeCommand = new QComputeCommand();
    //pComputeCommand->setWorkGroupX(iWeightMatRows);

    pComputeEntity->addComponent(pComputeCommand);
    pComputeEntity->addComponent(pComputeMaterial);


    //Color attribute
    QAttribute *pYOutAttribute = new QAttribute();
    pYOutAttribute->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    pYOutAttribute->setDataType(Qt3DRender::QAttribute::Float);
    pYOutAttribute->setVertexSize(1);
    pYOutAttribute->setByteOffset(0);
    pYOutAttribute->setByteStride(1 * sizeof(float));
    pYOutAttribute->setName(QStringLiteral("YOutVec"));
    pYOutAttribute->setBuffer(pYOutBuffer);

    //custom mesh
    DISP3DLIB::CustomMesh *pCustomMesh = new DISP3DLIB::CustomMesh();
    MatrixX3f matVertColor = createVertColor(t_sensorSurfaceVV[0].rr, QColor(255,0,0,255));

    //Set renderable 3D entity mesh and color data
    pCustomMesh->setMeshData(t_sensorSurfaceVV[0].rr,
                                t_sensorSurfaceVV[0].nn,
                                t_sensorSurfaceVV[0].tris,
                                matVertColor,
                                Qt3DRender::QGeometryRenderer::Triangles);
    //add Custom Attribute
    pCustomMesh->addAttrib(pYOutAttribute);

    //mesh render entity
    Qt3DCore::QEntity *pMeshRenderEntity = new Qt3DCore::QEntity(rootEntity);


    //@TODO delete material and forward renderer
//    Qt3DRender::QMaterial *material = new Qt3DExtras::QPhongMaterial(rootEntity);
//    Qt3DExtras::QForwardRenderer *forwardRenderer = new Qt3DExtras::QForwardRenderer();


    Qt3DCore::QTransform *pTransform = new Qt3DCore::QTransform;
    pTransform->setScale3D(QVector3D(50.0, 50.0, 50.0));


    pMeshRenderEntity->addComponent(pCustomMesh);
    //pMeshRenderEntity->addComponent(material);
    pMeshRenderEntity->addComponent(pComputeMaterial);
    pMeshRenderEntity->addComponent(pTransform);



    Qt3DRender::QCamera *pCamera = new QCamera;
    pCamera->setProjectionType(QCameraLens::PerspectiveProjection);
    pCamera->setViewCenter(QVector3D(0, 0, 0));
    pCamera->setPosition(QVector3D(0, 0, 40.0));
    pCamera->setNearPlane(0.1f);
    pCamera->setFarPlane(1000.0f);
    pCamera->setFieldOfView(25.0f);
    pCamera->setAspectRatio(1.33f);


    pFramegraph->setCamera(pCamera);
//    camera->lens()->setPerspectiveProjection(45.0f, 16.0f/9.0f, 0.1f, 1000.0f);
//    camera->setPosition(QVector3D(0, 0, 40.0f));
//    camera->setViewCenter(QVector3D(0, 0, 0));
    //forwardRenderer->setCamera(camera);
//    pFramegraph->setCamera(camera);

    //Camera controller
    Qt3DExtras::QFirstPersonCameraController *pCamController = new Qt3DExtras::QFirstPersonCameraController(rootEntity);
    pCamController->setCamera(pCamera);



    //Configure view settings
    view.setRootEntity(rootEntity);

    view.setActiveFrameGraph(pFramegraph);
    //view.setActiveFrameGraph(forwardRenderer);
    view.renderSettings()->setRenderPolicy(Qt3DRender::QRenderSettings::Always);

    //pCamController->setCamera(view.camera());

    view.show();


    return a.exec();
}

QByteArray createVertexBufferFromBemSurface(const MNEBemSurface &tBemSurface)
{
    const int iVertNum = tBemSurface.rr.rows();
    const int iVertSize = tBemSurface.rr.cols();

    QByteArray bufferData;
    bufferData.resize(iVertNum * iVertSize * (int)sizeof(float));
    float *rawVertexArray = reinterpret_cast<float *>(bufferData.data());

    int idxVert = 0;
    for(int i = 0; i < tBemSurface.rr.rows(); ++i) {
        rawVertexArray[idxVert++] = (tBemSurface.rr(i,0));
        rawVertexArray[idxVert++] = (tBemSurface.rr(i,1));
        rawVertexArray[idxVert++] = (tBemSurface.rr(i,2));
    }

    return bufferData;
}

QByteArray createColorBuffer(const int iVertNum)
{
    const int iBufferSize = iVertNum;
    QByteArray bufferData;
    bufferData.resize(iBufferSize * (int)sizeof(float));
    float *rawVertexArray = reinterpret_cast<float *>(bufferData.data());

    //Set default values
    for(int i = 0; i < iBufferSize; ++i)
    {
        rawVertexArray[i] = 1.0f;
    }
    return bufferData;
}

MatrixX3f createVertColor(const MatrixXf& vertices, const QColor& color)
{
    MatrixX3f matColor(vertices.rows(),3);

    for(int i = 0; i < matColor.rows(); ++i) {
        matColor(i,0) = color.redF();
        matColor(i,1) = color.greenF();
        matColor(i,2) = color.blueF();
    }

    return matColor;
}

QByteArray createWeightMatBuffer(QSharedPointer<SparseMatrix<double> > tInterpolationMatrix)
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
