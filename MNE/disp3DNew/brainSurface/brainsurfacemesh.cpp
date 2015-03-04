//=============================================================================================================
/**
* @file     brainsurfacemesh.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Implementationof BRAINSURFACEMESH which holds the data of one hemisphere in form of a mesh.
*
*/

#ifndef _USE_MATH_DEFINES
# define _USE_MATH_DEFINES // For MSVC
#endif


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainsurfacemesh.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include "qtorusmesh.h"
#include "qbuffer.h"
#include "qattribute.h"
#include "qmeshdata.h"

#include <cmath>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DNEWLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainSurfaceMesh::BrainSurfaceMesh(QNode *parent)
: QAbstractMesh(parent)
{
    update();
}


//*************************************************************************************************************

BrainSurfaceMesh::BrainSurfaceMesh(const Surface &surf, const QMap<int, QColor> &qmVertexColor, QNode *parent)
: QAbstractMesh(parent)
, m_surface(surf)
, m_qmVertexColor(qmVertexColor)
{
    update();
}


//*************************************************************************************************************

void BrainSurfaceMesh::copy(const QNode *ref)
{
    QAbstractMesh::copy(ref);
    const BrainSurfaceMesh *mesh = static_cast<const BrainSurfaceMesh*>(ref);
    m_surface = mesh->m_surface;
    m_qmVertexColor = mesh->m_qmVertexColor;
}


//*************************************************************************************************************

QAbstractMeshFunctorPtr BrainSurfaceMesh::meshFunctor() const
{
    return QAbstractMeshFunctorPtr(new BrainSurfaceMeshFunctor(m_surface, m_qmVertexColor));
}


//*************************************************************************************************************

void BrainSurfaceMesh::updateActivation(const QMap<int, QColor> &qmVertexColor)
{
    //std::cout<<"START::BrainSurfaceMesh::updateActivation"<<std::endl;
    //std::cout<<"vertexColor.size()"<<qmVertexColor.size()<<std::endl;
    //std::cout<<"m_qlVertexColor.size()"<<m_qmVertexColor.size()<<std::endl;
    //std::cout<<"m_surface.rr().cols()"<<m_surface.rr().rows()<<std::endl;

    if(qmVertexColor.size() != m_qmVertexColor.size()) {
        std::cout<<"newly provided colors from source estimate do not match loaded number of vertices"<<std::endl;
        std::cout<<"vertexColor.size()"<<qmVertexColor.size()<<std::endl;
        std::cout<<"m_qlVertexColor.size()"<<m_qmVertexColor.size()<<std::endl;

        return;
    }

    m_qmVertexColor = qmVertexColor;

    update();
    //std::cout<<"END::BrainSurfaceMesh::updateActivation"<<std::endl;
}


//*************************************************************************************************************

int BrainSurfaceMesh::getNumberOfVertices()
{
    return m_surface.rr().rows();
}


//*************************************************************************************************************

BrainSurfaceMeshFunctor::BrainSurfaceMeshFunctor(const Surface &surf, const QMap<int, QColor> &qmVertexColor)
: QAbstractMeshFunctor()
, m_surface(surf)
, m_qmVertexColor(qmVertexColor)
{
}


//*************************************************************************************************************

QMeshDataPtr BrainSurfaceMeshFunctor::operator ()()
{
    m_qMeshDataPtr = createSurfaceMesh(m_surface, m_qmVertexColor);
    return m_qMeshDataPtr;
}


//*************************************************************************************************************

bool BrainSurfaceMeshFunctor::operator ==(const QAbstractMeshFunctor &other) const
{
    const BrainSurfaceMeshFunctor *otherFunctor = dynamic_cast<const BrainSurfaceMeshFunctor *>(&other);
//    if (otherFunctor != Q_NULLPTR)
//        return (otherFunctor->m_surface == m_surface &&
//                otherFunctor->m_qlVertexColor == m_qlVertexColor);

    return false;
}

//*************************************************************************************************************

QMeshDataPtr BrainSurfaceMeshFunctor::createSurfaceMesh(const Surface &surface, const QMap<int, QColor> &qmVertexColor)
{
    QMeshDataPtr mesh(new QMeshData(QMeshData::Triangles));

    //Return if empty surface
    if(surface.isEmpty() == -1) {
        std::cout<<"BrainSurfaceMesh - createSurfaceMesh() - Given surface is empty"<<std::endl;
        return mesh;
    }

    //Create opengl data structure
    Matrix3Xf vertices = surface.rr().transpose();
    Matrix3Xf normals = surface.nn().transpose();
    Matrix3Xi faces = surface.tris().transpose();

    int nVerts  = vertices.cols();

    quint32 elementSizeVertNorm = 3 + 3; // vec3 pos, vec3 normal
    quint32 elementSizeColor = 3; // vec3 color
    quint32 strideVertNorm = elementSizeVertNorm * sizeof(float);
    quint32 strideColor = elementSizeColor * sizeof(float);

    QByteArray bufferBytesVertNorm;
    bufferBytesVertNorm.resize(strideVertNorm * nVerts);

    float* fptrVertNorm = reinterpret_cast<float*>(bufferBytesVertNorm.data());

    QByteArray bufferBytesColor;
    bufferBytesColor.resize(strideColor * nVerts);

    float* fptrColor = reinterpret_cast<float*>(bufferBytesColor.data());

    for(int i = 0; i<nVerts; i++) {
        //position x y z
        *fptrVertNorm++ = vertices(0,i);
        *fptrVertNorm++ = vertices(1,i);
        *fptrVertNorm++ = vertices(2,i);

        //normals x y z
        *fptrVertNorm++ = normals(0,i);
        *fptrVertNorm++ = normals(1,i);
        *fptrVertNorm++ = normals(2,i);

        //color rgb
        *fptrColor++ = (float)qmVertexColor[i].redF();
        *fptrColor++ = (float)qmVertexColor[i].greenF();
        *fptrColor++ = (float)qmVertexColor[i].blueF();

        //std::cout<<qmVertexColor[i].redF()<<" "<<qmVertexColor[i].greenF()<<" "<<qmVertexColor[i].blueF()<<std::endl;
    }

    //Create OpenGL buffers
    BufferPtr bufVertNorm(new Buffer(QOpenGLBuffer::VertexBuffer));
    bufVertNorm->setUsage(QOpenGLBuffer::StaticDraw);
    bufVertNorm->setData(bufferBytesVertNorm);

    BufferPtr bufColor(new Buffer(QOpenGLBuffer::VertexBuffer));
    bufColor->setUsage(QOpenGLBuffer::StaticDraw);
    bufColor->setData(bufferBytesColor);

    //Set vertices to OpenGL buffer
    mesh->addAttribute(QMeshData::defaultPositionAttributeName(), QAbstractAttributePtr(new Attribute(bufVertNorm, GL_FLOAT_VEC3, nVerts, 0, strideVertNorm)));
    quint32 offset = sizeof(float) * 3;

    //Set normals to OpenGL buffer
    mesh->addAttribute(QMeshData::defaultNormalAttributeName(), QAbstractAttributePtr(new Attribute(bufVertNorm, GL_FLOAT_VEC3, nVerts, offset, strideVertNorm)));

    //Set color to OpenGL buffer
    mesh->addAttribute(QMeshData::defaultColorAttributeName(), QAbstractAttributePtr(new Attribute(bufColor, GL_FLOAT_VEC3, nVerts, 0, strideColor)));

    //Generate faces out of tri information
    QByteArray indexBytes;
    int number_faces = faces.cols();
    int indices = number_faces * 3;

    indexBytes.resize(indices * sizeof(quint32));
    quint32* indexPtr = reinterpret_cast<quint32*>(indexBytes.data());

    for(int i = 0; i < number_faces; i++)
    {
        *indexPtr++ = faces(0,i);
        *indexPtr++ = faces(1,i);
        *indexPtr++ = faces(2,i);
    }

    BufferPtr indexBuffer(new Buffer(QOpenGLBuffer::IndexBuffer));
    indexBuffer->setUsage(QOpenGLBuffer::StaticDraw);
    indexBuffer->setData(indexBytes);
    mesh->setIndexAttribute(AttributePtr(new Attribute(indexBuffer, GL_UNSIGNED_INT, indices, 0, 0)));

    mesh->computeBoundsFromAttribute(QMeshData::defaultPositionAttributeName());

    //std::cout<<"Created QMeshData"<<std::endl;

    return mesh;
}

