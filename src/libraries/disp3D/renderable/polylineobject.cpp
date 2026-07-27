//=============================================================================================================
/**
 * @file     polylineobject.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.9
 * @date     July, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @brief    PolylineObject class definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "polylineobject.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMatrix4x4>
#include <QQuaternion>
#include <QVector3D>
#include <QtMath>
#include <rhi/qrhi.h>

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

struct PolylineObject::GpuResources
{
    std::unique_ptr<QRhiBuffer> vertexBuffer;
    std::unique_ptr<QRhiBuffer> indexBuffer;
    std::unique_ptr<QRhiBuffer> instanceBuffer;
};

//=============================================================================================================

PolylineObject::PolylineObject()
: m_iIndexCount(0)
, m_iInstanceCount(0)
, m_fRadius(0.0015f)
, m_startColor(QColor(60, 120, 255))
, m_endColor(QColor(255, 80, 40))
, m_bVisible(true)
, m_bGeometryDirty(true)
, m_bInstancesDirty(false)
, m_gpu(std::make_unique<GpuResources>())
{
    createSegmentGeometry();
}

//=============================================================================================================

PolylineObject::~PolylineObject() = default;

//=============================================================================================================

void PolylineObject::setPoints(const QVector<Eigen::Vector3f>& vecPoints)
{
    // A single point has no segment, so it is treated the same as no data at
    // all rather than drawn as a degenerate line.
    if(vecPoints.size() < 2) {
        clear();
        return;
    }

    m_vecPoints = vecPoints;
    buildInstances();
}

//=============================================================================================================

void PolylineObject::clear()
{
    m_vecPoints.clear();
    m_instanceData.clear();
    m_iInstanceCount = 0;
    m_bInstancesDirty = false;
}

//=============================================================================================================

void PolylineObject::setRadius(float fRadius)
{
    m_fRadius = fRadius;
    buildInstances();
}

//=============================================================================================================

void PolylineObject::setGradient(const QColor& startColor,
                                 const QColor& endColor)
{
    m_startColor = startColor;
    m_endColor = endColor;
    buildInstances();
}

//=============================================================================================================

void PolylineObject::createSegmentGeometry()
{
    if(!m_vertexData.isEmpty()) {
        return;
    }

    // Unit cylinder along Y, height 1 and radius 1, scaled and rotated per
    // instance. Eight segments is enough for a thin line and keeps the vertex
    // count low, since a long path can have thousands of instances.
    const int iSegments = 8;
    const float fHalfHeight = 0.5f;

    std::vector<VertexData> vertices;
    std::vector<quint32> indices;

    for(int i = 0; i < iSegments; ++i) {
        const float fAngle = 2.0f * static_cast<float>(M_PI) * static_cast<float>(i) / static_cast<float>(iSegments);
        const float x = std::cos(fAngle);
        const float z = std::sin(fAngle);

        QVector3D normal(x, 0.0f, z);
        normal.normalize();

        vertices.push_back({x,  fHalfHeight, z, normal.x(), normal.y(), normal.z()});
        vertices.push_back({x, -fHalfHeight, z, normal.x(), normal.y(), normal.z()});
    }

    for(int i = 0; i < iSegments; ++i) {
        const quint32 uTop = static_cast<quint32>(i * 2);
        const quint32 uBottom = uTop + 1;
        const quint32 uNextTop = static_cast<quint32>(((i + 1) % iSegments) * 2);
        const quint32 uNextBottom = uNextTop + 1;

        indices.push_back(uTop);
        indices.push_back(uBottom);
        indices.push_back(uNextTop);

        indices.push_back(uNextTop);
        indices.push_back(uBottom);
        indices.push_back(uNextBottom);
    }

    m_vertexData = QByteArray(reinterpret_cast<const char*>(vertices.data()),
                              static_cast<qsizetype>(vertices.size() * sizeof(VertexData)));
    m_indexData = QByteArray(reinterpret_cast<const char*>(indices.data()),
                             static_cast<qsizetype>(indices.size() * sizeof(quint32)));
    m_iIndexCount = static_cast<int>(indices.size());
    m_bGeometryDirty = true;
}

//=============================================================================================================

void PolylineObject::buildInstances()
{
    if(m_vecPoints.size() < 2) {
        m_instanceData.clear();
        m_iInstanceCount = 0;
        return;
    }

    const int iSegmentCount = m_vecPoints.size() - 1;
    std::vector<InstanceData> instances;
    instances.reserve(static_cast<size_t>(iSegmentCount));

    const float fLastStep = static_cast<float>(iSegmentCount - 1);

    for(int i = 0; i < iSegmentCount; ++i) {
        const Eigen::Vector3f& vecStart = m_vecPoints.at(i);
        const Eigen::Vector3f& vecEnd = m_vecPoints.at(i + 1);

        const QVector3D start(vecStart.x(), vecStart.y(), vecStart.z());
        const QVector3D end(vecEnd.x(), vecEnd.y(), vecEnd.z());

        const QVector3D direction = end - start;
        const float fLength = direction.length();

        // Two consecutive fits can land on the same position when the subject
        // does not move. Such a segment has no direction to orient a cylinder
        // by, so it is skipped instead of producing a NaN rotation.
        if(qFuzzyIsNull(fLength)) {
            continue;
        }

        QMatrix4x4 model;
        model.translate((start + end) * 0.5f);
        model.rotate(QQuaternion::rotationTo(QVector3D(0.0f, 1.0f, 0.0f),
                                             direction / fLength));
        model.scale(m_fRadius, fLength, m_fRadius);

        InstanceData instance{};
        memcpy(instance.model, model.constData(), 16 * sizeof(float));

        // Grade from the start colour to the end colour so the direction the
        // path was travelled in is visible without extra decoration.
        const float fRatio = (fLastStep > 0.0f) ? (static_cast<float>(i) / fLastStep) : 0.0f;
        instance.color[0] = static_cast<float>(m_startColor.redF() + (m_endColor.redF() - m_startColor.redF()) * fRatio);
        instance.color[1] = static_cast<float>(m_startColor.greenF() + (m_endColor.greenF() - m_startColor.greenF()) * fRatio);
        instance.color[2] = static_cast<float>(m_startColor.blueF() + (m_endColor.blueF() - m_startColor.blueF()) * fRatio);
        instance.color[3] = 1.0f;
        instance.isSelected = 0.0f;

        instances.push_back(instance);
    }

    m_instanceData = QByteArray(reinterpret_cast<const char*>(instances.data()),
                                static_cast<qsizetype>(instances.size() * sizeof(InstanceData)));
    m_iInstanceCount = static_cast<int>(instances.size());
    m_bInstancesDirty = m_iInstanceCount > 0;
}

//=============================================================================================================

void PolylineObject::updateBuffers(QRhi *rhi, QRhiResourceUpdateBatch *u)
{
    if(m_bGeometryDirty) {
        if(!m_gpu->vertexBuffer) {
            m_gpu->vertexBuffer.reset(rhi->newBuffer(QRhiBuffer::Immutable,
                                                     QRhiBuffer::VertexBuffer,
                                                     m_vertexData.size()));
            m_gpu->vertexBuffer->create();
        }
        if(!m_gpu->indexBuffer) {
            m_gpu->indexBuffer.reset(rhi->newBuffer(QRhiBuffer::Immutable,
                                                    QRhiBuffer::IndexBuffer,
                                                    m_indexData.size()));
            m_gpu->indexBuffer->create();
        }

        u->uploadStaticBuffer(m_gpu->vertexBuffer.get(), m_vertexData.constData());
        u->uploadStaticBuffer(m_gpu->indexBuffer.get(), m_indexData.constData());
        m_bGeometryDirty = false;
    }

    if(m_bInstancesDirty && m_iInstanceCount > 0) {
        const int iRequiredSize = m_instanceData.size();

        if(!m_gpu->instanceBuffer || m_gpu->instanceBuffer->size() < static_cast<quint32>(iRequiredSize)) {
            m_gpu->instanceBuffer.reset(rhi->newBuffer(QRhiBuffer::Dynamic,
                                                       QRhiBuffer::VertexBuffer,
                                                       iRequiredSize));
            m_gpu->instanceBuffer->create();
        }

        u->updateDynamicBuffer(m_gpu->instanceBuffer.get(), 0, iRequiredSize, m_instanceData.constData());
        m_bInstancesDirty = false;
    }
}

//=============================================================================================================

QRhiBuffer* PolylineObject::vertexBuffer() const
{
    return m_gpu->vertexBuffer.get();
}

//=============================================================================================================

QRhiBuffer* PolylineObject::indexBuffer() const
{
    return m_gpu->indexBuffer.get();
}

//=============================================================================================================

QRhiBuffer* PolylineObject::instanceBuffer() const
{
    return m_gpu->instanceBuffer.get();
}
