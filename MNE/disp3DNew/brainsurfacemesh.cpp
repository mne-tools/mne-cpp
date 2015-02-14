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

BrainSurfaceMesh::BrainSurfaceMesh(const Surface &surf, QNode *parent)
: QAbstractMesh(parent)
, m_surface(surf)
{
    update();
}


//*************************************************************************************************************

void BrainSurfaceMesh::copy(const QNode *ref)
{
    QAbstractMesh::copy(ref);
    const BrainSurfaceMesh *mesh = static_cast<const BrainSurfaceMesh*>(ref);
//    d_func()->m_surface = mesh->d_func()->m_surface;
}

//*************************************************************************************************************

QMeshDataPtr createHemisphereMesh(const Surface &surface)
{
    QMeshDataPtr mesh(new QMeshData(QMeshData::Triangles));

    //Return if empty surface
    if(surface.isEmpty() == -1)
        return mesh;

    //Create opengl data structure
    Matrix3Xf vertices = surface.rr().transpose();
    Matrix3Xf normals = surface.nn().transpose();
    Matrix3Xi faces = surface.tris().transpose();

    int nVerts  = vertices.cols();

    quint32 elementSize = 3 + 2 + 3; // vec3 pos, vec2 texCoord, vec3 normal
    quint32 stride = elementSize * sizeof(float);

    QByteArray bufferBytes;
    bufferBytes.resize(stride * nVerts);

    float* fptr = reinterpret_cast<float*>(bufferBytes.data());

    for(int i = 0; i<vertices.cols(); i++) {
        //position x y z
        *fptr++ = vertices(0,i);
        *fptr++ = vertices(1,i);
        *fptr++ = vertices(2,i);

        //texture u v
        *fptr++ = 1;
        *fptr++ = 1;

        //normals x y z
        *fptr++ = normals(0,i);
        *fptr++ = normals(1,i);
        *fptr++ = normals(2,i);
    }

    //Create OpenGL buffer
    BufferPtr buf(new Buffer(QOpenGLBuffer::VertexBuffer));
    buf->setUsage(QOpenGLBuffer::StaticDraw);
    buf->setData(bufferBytes);

    //Set vertices to OpenGL buffer
    mesh->addAttribute(QMeshData::defaultPositionAttributeName(), QAbstractAttributePtr(new Attribute(buf, GL_FLOAT_VEC3, nVerts, 0, stride)));
    quint32 offset = sizeof(float) * 3;

    //Set textures to OpenGL buffer
    mesh->addAttribute(QMeshData::defaultTextureCoordinateAttributeName(), QAbstractAttributePtr(new Attribute(buf, GL_FLOAT_VEC2, nVerts, offset, stride)));
    offset += sizeof(float) * 2;

    //Set normals to OpenGL buffer
    mesh->addAttribute(QMeshData::defaultNormalAttributeName(), QAbstractAttributePtr(new Attribute(buf, GL_FLOAT_VEC3, nVerts, offset, stride)));
    offset += sizeof(float) * 3;

    //Generate faces out of tri information
    QByteArray indexBytes;
    int number_faces = faces.cols(); // two tris per side, for all rings
    int indices = number_faces * 3;

    indexBytes.resize(indices * sizeof(quint16));
    quint16* indexPtr = reinterpret_cast<quint16*>(indexBytes.data());

    for(int i = 0; i < number_faces; i++)
    {
        *indexPtr++ = faces(0,i);
        *indexPtr++ = faces(1,i);
        *indexPtr++ = faces(2,i);
    }

    BufferPtr indexBuffer(new Buffer(QOpenGLBuffer::IndexBuffer));
    indexBuffer->setUsage(QOpenGLBuffer::StaticDraw);
    indexBuffer->setData(indexBytes);
    mesh->setIndexAttribute(AttributePtr(new Attribute(indexBuffer, GL_UNSIGNED_SHORT, indices, 0, 0)));

    mesh->computeBoundsFromAttribute(QMeshData::defaultPositionAttributeName());

//    //QTorusMesh example
//    int rings = 16;
//    int slices = 1;
//    float radius = 5.0;
//    float minorRadius = 1.0;
//    int sides = 6;

//    int nVerts  = sides * ( rings + 1 );
//    QByteArray bufferBytes;
//    // vec3 pos, vec2 texCoord, vec3 normal
//    quint32 elementSize = 3 + 2 + 3;
//    quint32 stride = elementSize * sizeof(float);
//    bufferBytes.resize(stride * nVerts);

//    float* fptr = reinterpret_cast<float*>(bufferBytes.data());

//    float ringFactor = (M_PI * 2) / static_cast<float>( rings );
//    float sideFactor = (M_PI * 2) / static_cast<float>( sides );

//    for ( int ring = 0; ring <= rings; ring++ )
//    {
//        float u = ring * ringFactor;
//        float cu = cos( u );
//        float su = sin( u );

//        for ( int side = 0; side < sides; side++ )
//        {
//            float v = side * sideFactor;
//            float cv = cos( v );
//            float sv = sin( v );
//            float r = ( radius + minorRadius * cv );

//            *fptr++ = r * cu;
//            *fptr++ = r * su;
//            *fptr++ = minorRadius * sv;


//            *fptr++ = u / (M_PI * 2);
//            *fptr++ = v / (M_PI * 2);

//            QVector3D n(cv * cu * r, cv * su * r, sv * r);
//            n.normalize();
//            *fptr++ = n.x();
//            *fptr++ = n.y();
//            *fptr++ = n.z();
//        }
//    }

//    BufferPtr buf(new Buffer(QOpenGLBuffer::VertexBuffer));
//    buf->setUsage(QOpenGLBuffer::StaticDraw);
//    buf->setData(bufferBytes);

//    mesh->addAttribute(QMeshData::defaultPositionAttributeName(), QAbstractAttributePtr(new Attribute(buf, GL_FLOAT_VEC3, nVerts, 0, stride)));
//    quint32 offset = sizeof(float) * 3;

//    mesh->addAttribute(QMeshData::defaultTextureCoordinateAttributeName(), QAbstractAttributePtr(new Attribute(buf, GL_FLOAT_VEC2, nVerts, offset, stride)));
//    offset += sizeof(float) * 2;

//    mesh->addAttribute(QMeshData::defaultNormalAttributeName(), QAbstractAttributePtr(new Attribute(buf, GL_FLOAT_VEC3, nVerts, offset, stride)));
//    offset += sizeof(float) * 3;

//    QByteArray indexBytes;
//    int faces = (sides * 2) * rings; // two tris per side, for all rings
//    int indices = faces * 3;
//    Q_ASSERT(indices < 65536);
//    indexBytes.resize(indices * sizeof(quint16));
//    quint16* indexPtr = reinterpret_cast<quint16*>(indexBytes.data());

//    for ( int ring = 0; ring < rings; ring++ )
//    {
//        int ringStart = ring * sides;
//        int nextRingStart = ( ring + 1 ) * sides;
//        for ( int side = 0; side < sides; side++ )
//        {
//            int nextSide = ( side + 1 ) % sides;
//            *indexPtr++ = ( ringStart + side );
//            *indexPtr++ = ( nextRingStart + side );
//            *indexPtr++ = ( nextRingStart + nextSide );
//            *indexPtr++ = ringStart + side;
//            *indexPtr++ = nextRingStart + nextSide;
//            *indexPtr++ = ( ringStart + nextSide );
//        }
//    }

//    BufferPtr indexBuffer(new Buffer(QOpenGLBuffer::IndexBuffer));
//    indexBuffer->setUsage(QOpenGLBuffer::StaticDraw);
//    indexBuffer->setData(indexBytes);
//    mesh->setIndexAttribute(AttributePtr(new Attribute(indexBuffer, GL_UNSIGNED_SHORT, indices, 0, 0)));

//    mesh->computeBoundsFromAttribute(QMeshData::defaultPositionAttributeName());

    return mesh;
}


//*************************************************************************************************************

QAbstractMeshFunctorPtr BrainSurfaceMesh::meshFunctor() const
{
    return QAbstractMeshFunctorPtr(new BrainSurfaceMeshFunctor(m_surface));
}


//*************************************************************************************************************

BrainSurfaceMeshFunctor::BrainSurfaceMeshFunctor(const Surface &surf)
: QAbstractMeshFunctor()
, m_surface(surf)
{
}


//*************************************************************************************************************

QMeshDataPtr BrainSurfaceMeshFunctor::operator ()()
{
    return createHemisphereMesh(m_surface);
}


//*************************************************************************************************************

bool BrainSurfaceMeshFunctor::operator ==(const QAbstractMeshFunctor &other) const
{
    const BrainSurfaceMeshFunctor *otherFunctor = dynamic_cast<const BrainSurfaceMeshFunctor *>(&other);
    if (otherFunctor != Q_NULLPTR)
        return true;
    return false;
}


//*************************************************************************************************************



