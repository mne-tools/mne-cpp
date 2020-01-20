//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @version  dev
 * @date     September, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Christoph Dinh. All rights reserved.
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
 * @brief     Example of reading forward solution and plotting it with new qt 3d
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QGuiApplication>

#include "common/window.h"
#include <Qt3DCore/QEntity>
#include <Qt3DCore/QCamera>
#include <Qt3DCore/QCameraLens>
#include <Qt3DCore/QTransform>
#include <Qt3DCore/QLookAtTransform>
#include <Qt3DCore/QScaleTransform>
#include <Qt3DCore/QRotateTransform>
#include <Qt3DCore/QTranslateTransform>
#include <Qt3DCore/QAspectEngine>

#include <Qt3DInput/QInputAspect>

#include <Qt3DRenderer/QStateSet>
#include <Qt3DRenderer/QRenderAspect>
#include <Qt3DRenderer/QFrameGraph>
#include <Qt3DRenderer/QForwardRenderer>
#include <Qt3DRenderer/QPerVertexColorMaterial>

#include <Qt3DRenderer/QGeometryRenderer>
#include <Qt3DRenderer/QGeometry>
#include <Qt3DRenderer/QAttribute>
#include <Qt3DRenderer/QBuffer>

#include <QPropertyAnimation>


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>
#include <mne/mne.h>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;


//*************************************************************************************************************
//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);



    QFile t_fileForwardSolution("./MNE-sample-data/MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif");
    MNEForwardSolution t_ForwardSolution(t_fileForwardSolution);

    Window view;

    Qt3D::QAspectEngine engine;
    engine.registerAspect(new Qt3D::QRenderAspect());
    Qt3D::QInputAspect *input = new Qt3D::QInputAspect;
    engine.registerAspect(input);
    engine.initialize();
    QVariantMap data;
    data.insert(QStringLiteral("surface"), QVariant::fromValue(static_cast<QSurface *>(&view)));
    data.insert(QStringLiteral("eventSource"), QVariant::fromValue(&view));
    engine.setData(data);

    // Root entity
    Qt3D::QEntity *rootEntity = new Qt3D::QEntity();

    // Camera
    Qt3D::QCamera *cameraEntity = new Qt3D::QCamera(rootEntity);

    cameraEntity->lens()->setPerspectiveProjection(40.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    cameraEntity->setPosition(QVector3D( 0, 0, -1.0f));
    cameraEntity->setUpVector(QVector3D(0, 1, 0));
    cameraEntity->setViewCenter(QVector3D(0, 0, 0));
    input->setCamera(cameraEntity);

    // FrameGraph
    Qt3D::QFrameGraph *frameGraph = new Qt3D::QFrameGraph();
    Qt3D::QForwardRenderer *forwardRenderer = new Qt3D::QForwardRenderer();
    forwardRenderer->setClearColor(QColor::fromRgbF(0.0, 0.5, 1.0, 1.0));
    forwardRenderer->setCamera(cameraEntity);
    frameGraph->setActiveFrameGraph(forwardRenderer);

    // Material
    Qt3D::QMaterial *material = new Qt3D::QPerVertexColorMaterial(rootEntity);

    // Torus
    Qt3D::QEntity *customMeshEntity = new Qt3D::QEntity(rootEntity);

    // Transform
    Qt3D::QTransform *transform = new Qt3D::QTransform;
    Qt3D::QScaleTransform *scaleTransform = new Qt3D::QScaleTransform;
    scaleTransform->setScale(8.0f);
    transform->addTransform(scaleTransform);

    // Custom Mesh (TetraHedron)
    Qt3D::QGeometryRenderer *customMeshRenderer = new Qt3D::QGeometryRenderer;
    Qt3D::QGeometry *customGeometry = new Qt3D::QGeometry(customMeshRenderer);

    Qt3D::QBuffer *vertexDataBuffer = new Qt3D::QBuffer(Qt3D::QBuffer::VertexBuffer, customGeometry);
    Qt3D::QBuffer *indexDataBuffer = new Qt3D::QBuffer(Qt3D::QBuffer::IndexBuffer, customGeometry);

    //

    qDebug() << "rr rows: " << t_ForwardSolution.src[0].rr.rows() << "; cols: " << t_ForwardSolution.src[0].rr.cols();

    qDebug() << "nn rows: " << t_ForwardSolution.src[0].nn.rows() << "; cols: " << t_ForwardSolution.src[0].nn.cols();

    int num_positions = t_ForwardSolution.src[0].rr.rows();


    // 4 distinct vertices
    QByteArray vertexBufferData;
    vertexBufferData.resize(num_positions * (3 + 3 + 3) * sizeof(float));

    QVector3D white(1.0f, 1.0f, 1.0f);

    QVector3D blue(0.0f, 0.0f, 1.0f);

    QVector<QVector3D> vertices;


    int color_debug_count = 0;
    //This is obsolete GoTo 1:
    for(qint32 i = 0; i < num_positions; ++i)
    {
        // Vertex
        QVector3D v(t_ForwardSolution.src[0].rr(i,0), t_ForwardSolution.src[0].rr(i,1), t_ForwardSolution.src[0].rr(i,2));

        // Vector Normal
        QVector3D n(t_ForwardSolution.src[0].nn(i,0), t_ForwardSolution.src[0].nn(i,1), t_ForwardSolution.src[0].nn(i,2));

        if(color_debug_count % 2 == 0)
        {
            vertices << v << n << white;
        }
        else
        {
            vertices << v << n << blue;
        }

        ++color_debug_count;
    }

    qDebug() << "vertices generated";

    float *rawVertexArray = reinterpret_cast<float *>(vertexBufferData.data());
    int idx = 0;

    Q_FOREACH (const QVector3D &v, vertices) {
        rawVertexArray[idx++] = v.x();
        rawVertexArray[idx++] = v.y();
        rawVertexArray[idx++] = v.z();
    }

    qDebug() << "raw vertex array generated";
    qDebug() << "tri rows: " << t_ForwardSolution.src[0].tris.rows() << "; cols:" << t_ForwardSolution.src[0].tris.cols();

    int num_tri_elements = t_ForwardSolution.src[0].tris.rows() * t_ForwardSolution.src[0].tris.cols();


    // Indices (12)
    QByteArray indexBufferData;
    indexBufferData.resize(num_tri_elements * sizeof(ushort));
    ushort *rawIndexArray = reinterpret_cast<ushort *>(indexBufferData.data());

    qint32 count = 0;
    for(qint32 i = 0; i < t_ForwardSolution.src[0].tris.rows(); ++i)
    {
        rawIndexArray[count++] = t_ForwardSolution.src[0].tris(i,0);
        rawIndexArray[count++] = t_ForwardSolution.src[0].tris(i,1);
        rawIndexArray[count++] = t_ForwardSolution.src[0].tris(i,2);
    }

    vertexDataBuffer->setData(vertexBufferData);
    indexDataBuffer->setData(indexBufferData);

    qDebug() << "### 1";

    // Attributes
    Qt3D::QAttribute *positionAttribute = new Qt3D::QAttribute();
    positionAttribute->setAttributeType(Qt3D::QAttribute::VertexAttribute);
    positionAttribute->setBuffer(vertexDataBuffer);
    positionAttribute->setDataType(Qt3D::QAttribute::Float);
    positionAttribute->setDataSize(3);
    positionAttribute->setByteOffset(0);
    positionAttribute->setByteStride(9 * sizeof(float));
    positionAttribute->setCount( num_positions );
    positionAttribute->setName(Qt3D::QAttribute::defaultPositionAttributeName());

    qDebug() << "### 2";

    Qt3D::QAttribute *normalAttribute = new Qt3D::QAttribute();
    normalAttribute->setAttributeType(Qt3D::QAttribute::VertexAttribute);
    normalAttribute->setBuffer(vertexDataBuffer);
    normalAttribute->setDataType(Qt3D::QAttribute::Float);
    normalAttribute->setDataSize(3);
    normalAttribute->setByteOffset(3 * sizeof(float));
    normalAttribute->setByteStride(9 * sizeof(float));
    normalAttribute->setCount( num_positions );
    normalAttribute->setName(Qt3D::QAttribute::defaultNormalAttributeName());

    qDebug() << "### 3";

    Qt3D::QAttribute *colorAttribute = new Qt3D::QAttribute();
    colorAttribute->setAttributeType(Qt3D::QAttribute::VertexAttribute);
    colorAttribute->setBuffer(vertexDataBuffer);
    colorAttribute->setDataType(Qt3D::QAttribute::Float);
    colorAttribute->setDataSize(3);
    colorAttribute->setByteOffset(6 * sizeof(float));
    colorAttribute->setByteStride(9 * sizeof(float));
    colorAttribute->setCount( num_positions );
    colorAttribute->setName(Qt3D::QAttribute::defaultColorAttributeName());

    qDebug() << "### 4";

    Qt3D::QAttribute *indexAttribute = new Qt3D::QAttribute();
    indexAttribute->setAttributeType(Qt3D::QAttribute::IndexAttribute);
    indexAttribute->setBuffer(indexDataBuffer);
    indexAttribute->setDataType(Qt3D::QAttribute::UnsignedShort);
    indexAttribute->setDataSize(1);
    indexAttribute->setByteOffset(0);
    indexAttribute->setByteStride(0);
    indexAttribute->setCount(num_tri_elements);

    qDebug() << "### 5";

    customGeometry->addAttribute(positionAttribute);
    customGeometry->addAttribute(normalAttribute);
    customGeometry->addAttribute(colorAttribute);
    customGeometry->addAttribute(indexAttribute);

    customMeshRenderer->setInstanceCount(1);
    customMeshRenderer->setBaseVertex(0);
    customMeshRenderer->setBaseInstance(0);
    customMeshRenderer->setPrimitiveType(Qt3D::QGeometryRenderer::Triangles);
    customMeshRenderer->setGeometry(customGeometry);
    // t_ForwardSolution.src[0].tris.rows() faces of 3 points
    customMeshRenderer->setPrimitiveCount(num_tri_elements);

    customMeshEntity->addComponent(customMeshRenderer);
    customMeshEntity->addComponent(transform);
    customMeshEntity->addComponent(material);


    rootEntity->addComponent(frameGraph);

    engine.setRootEntity(rootEntity);
    view.show();





//    // vec3 for position
//    // vec3 for colors
//    // vec3 for normals

//    /*          2
//               /|\
//              / | \
//             / /3\ \
//             0/___\ 1
//    */

//    // 4 distinct vertices
//    QByteArray vertexBufferData;
//    vertexBufferData.resize(4 * (3 + 3 + 3) * sizeof(float));

//    // Vertices
//    QVector3D v0(-1.0f, 0.0f, -1.0f);
//    QVector3D v1(1.0f, 0.0f, -1.0f);
//    QVector3D v2(0.0f, 1.0f, 0.0f);
//    QVector3D v3(0.0f, 0.0f, 1.0f);

//    // Faces Normals
//    QVector3D n023 = QVector3D::normal(v0, v2, v3);
//    QVector3D n012 = QVector3D::normal(v0, v1, v2);
//    QVector3D n310 = QVector3D::normal(v3, v1, v0);
//    QVector3D n132 = QVector3D::normal(v1, v3, v2);

//    // Vector Normals
//    QVector3D n0 = QVector3D(n023 + n012 + n310).normalized();
//    QVector3D n1 = QVector3D(n132 + n012 + n310).normalized();
//    QVector3D n2 = QVector3D(n132 + n012 + n023).normalized();
//    QVector3D n3 = QVector3D(n132 + n310 + n023).normalized();

//    // Colors
//    QVector3D red(1.0f, 0.0f, 0.0f);
//    QVector3D green(0.0f, 1.0f, 0.0f);
//    QVector3D blue(0.0f, 0.0f, 1.0f);
//    QVector3D white(1.0f, 1.0f, 1.0f);

//    QVector<QVector3D> vertices = QVector<QVector3D>()
//            << v0 << n0 << green
//            << v1 << n1 << green
//            << v2 << n2 << green
//            << v3 << n3 << green;

//    float *rawVertexArray = reinterpret_cast<float *>(vertexBufferData.data());
//    int idx = 0;

//    Q_FOREACH (const QVector3D &v, vertices) {
//        rawVertexArray[idx++] = v.x();
//        rawVertexArray[idx++] = v.y();
//        rawVertexArray[idx++] = v.z();
//    }


//    // Indices (12)
//    QByteArray indexBufferData;
//    indexBufferData.resize(4 * 3 * sizeof(ushort));
//    ushort *rawIndexArray = reinterpret_cast<ushort *>(indexBufferData.data());

//    // Front
//    rawIndexArray[0] = 0;
//    rawIndexArray[1] = 1;
//    rawIndexArray[2] = 2;
//    // Bottom
//    rawIndexArray[3] = 3;
//    rawIndexArray[4] = 1;
//    rawIndexArray[5] = 0;
//    // Left
//    rawIndexArray[6] = 0;
//    rawIndexArray[7] = 2;
//    rawIndexArray[8] = 3;
//    // Right
//    rawIndexArray[9] = 1;
//    rawIndexArray[10] = 3;
//    rawIndexArray[11] = 2;

//    vertexDataBuffer->setData(vertexBufferData);
//    indexDataBuffer->setData(indexBufferData);


//    // Attributes
//    Qt3D::QAttribute *positionAttribute = new Qt3D::QAttribute();
//    positionAttribute->setAttributeType(Qt3D::QAttribute::VertexAttribute);
//    positionAttribute->setBuffer(vertexDataBuffer);
//    positionAttribute->setDataType(Qt3D::QAttribute::Float);
//    positionAttribute->setDataSize(3);
//    positionAttribute->setByteOffset(0);
//    positionAttribute->setByteStride(9 * sizeof(float));
//    positionAttribute->setCount(4);
//    positionAttribute->setName(Qt3D::QAttribute::defaultPositionAttributeName());

//    Qt3D::QAttribute *normalAttribute = new Qt3D::QAttribute();
//    normalAttribute->setAttributeType(Qt3D::QAttribute::VertexAttribute);
//    normalAttribute->setBuffer(vertexDataBuffer);
//    normalAttribute->setDataType(Qt3D::QAttribute::Float);
//    normalAttribute->setDataSize(3);
//    normalAttribute->setByteOffset(3 * sizeof(float));
//    normalAttribute->setByteStride(9 * sizeof(float));
//    normalAttribute->setCount(4);
//    normalAttribute->setName(Qt3D::QAttribute::defaultNormalAttributeName());

//    Qt3D::QAttribute *colorAttribute = new Qt3D::QAttribute();
//    colorAttribute->setAttributeType(Qt3D::QAttribute::VertexAttribute);
//    colorAttribute->setBuffer(vertexDataBuffer);
//    colorAttribute->setDataType(Qt3D::QAttribute::Float);
//    colorAttribute->setDataSize(3);
//    colorAttribute->setByteOffset(6 * sizeof(float));
//    colorAttribute->setByteStride(9 * sizeof(float));
//    colorAttribute->setCount(4);
//    colorAttribute->setName(Qt3D::QAttribute::defaultColorAttributeName());

//    Qt3D::QAttribute *indexAttribute = new Qt3D::QAttribute();
//    indexAttribute->setAttributeType(Qt3D::QAttribute::IndexAttribute);
//    indexAttribute->setBuffer(indexDataBuffer);
//    indexAttribute->setDataType(Qt3D::QAttribute::UnsignedShort);
//    indexAttribute->setDataSize(1);
//    indexAttribute->setByteOffset(0);
//    indexAttribute->setByteStride(0);
//    indexAttribute->setCount(12);

//    customGeometry->addAttribute(positionAttribute);
//    customGeometry->addAttribute(normalAttribute);
//    customGeometry->addAttribute(colorAttribute);
//    customGeometry->addAttribute(indexAttribute);

//    customMeshRenderer->setInstanceCount(1);
//    customMeshRenderer->setBaseVertex(0);
//    customMeshRenderer->setBaseInstance(0);
//    customMeshRenderer->setPrimitiveType(Qt3D::QGeometryRenderer::Triangles);
//    customMeshRenderer->setGeometry(customGeometry);
//    // 4 faces of 3 points
//    customMeshRenderer->setPrimitiveCount(12);

//    customMeshEntity->addComponent(customMeshRenderer);
//    customMeshEntity->addComponent(transform);
//    customMeshEntity->addComponent(material);

//    rootEntity->addComponent(frameGraph);

//    engine.setRootEntity(rootEntity);
//    view.show();

    return app.exec();
}
