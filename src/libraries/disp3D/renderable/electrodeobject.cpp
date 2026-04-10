//=============================================================================================================
/**
 * @file     electrodeobject.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
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
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    ElectrodeObject class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "electrodeobject.h"

#include <QtMath>

#include <limits>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ElectrodeObject::ElectrodeObject()
    : m_bbMin(std::numeric_limits<float>::max(),
              std::numeric_limits<float>::max(),
              std::numeric_limits<float>::max())
    , m_bbMax(std::numeric_limits<float>::lowest(),
              std::numeric_limits<float>::lowest(),
              std::numeric_limits<float>::lowest())
{
}

//=============================================================================================================

void ElectrodeObject::setShafts(const QVector<ElectrodeShaft>& shafts)
{
    m_shafts = shafts;
    m_selectedContact.clear();
    computeBoundingBox();
}

//=============================================================================================================

const QVector<ElectrodeShaft>& ElectrodeObject::shafts() const
{
    return m_shafts;
}

//=============================================================================================================

int ElectrodeObject::totalContactCount() const
{
    int count = 0;
    for (const auto& shaft : m_shafts)
        count += shaft.contacts.size();
    return count;
}

//=============================================================================================================

void ElectrodeObject::setContactValues(const QMap<QString, float>& values,
                                       const QColor& minColor,
                                       const QColor& maxColor)
{
    if (values.isEmpty())
        return;

    // Find global min/max across provided values
    float minVal = std::numeric_limits<float>::max();
    float maxVal = std::numeric_limits<float>::lowest();
    for (auto it = values.cbegin(); it != values.cend(); ++it) {
        if (it.value() < minVal) minVal = it.value();
        if (it.value() > maxVal) maxVal = it.value();
    }

    // Apply to contacts
    for (auto& shaft : m_shafts) {
        for (auto& contact : shaft.contacts) {
            auto it = values.find(contact.name);
            if (it != values.end()) {
                contact.value = it.value();
                contact.color = interpolateColor(it.value(), minVal, maxVal,
                                                 minColor, maxColor);
            }
        }
    }
}

//=============================================================================================================

void ElectrodeObject::selectContact(const QString& name)
{
    // Clear previous selection
    clearSelection();

    // Set new selection
    m_selectedContact = name;
    for (auto& shaft : m_shafts) {
        for (auto& contact : shaft.contacts) {
            if (contact.name == name) {
                contact.selected = true;
                return;
            }
        }
    }

    // Not found — clear name
    m_selectedContact.clear();
}

//=============================================================================================================

void ElectrodeObject::clearSelection()
{
    m_selectedContact.clear();
    for (auto& shaft : m_shafts) {
        for (auto& contact : shaft.contacts)
            contact.selected = false;
    }
}

//=============================================================================================================

QString ElectrodeObject::selectedContact() const
{
    return m_selectedContact;
}

//=============================================================================================================

void ElectrodeObject::generateShaftGeometry(QVector<float>& vertices,
                                            QVector<unsigned int>& indices,
                                            int cylinderSides) const
{
    vertices.clear();
    indices.clear();

    if (cylinderSides < 3)
        cylinderSides = 3;

    for (const auto& shaft : m_shafts) {
        if (shaft.contacts.size() < 2)
            continue;

        const QVector3D& tipPos  = shaft.contacts.first().position;
        const QVector3D& tailPos = shaft.contacts.last().position;
        const QVector3D axis = tailPos - tipPos;
        const float length = axis.length();
        if (length < 1e-6f)
            continue;

        const QVector3D axisNorm = axis.normalized();

        // Build a perpendicular basis
        QVector3D perp;
        if (qAbs(QVector3D::dotProduct(axisNorm, QVector3D(0, 1, 0))) < 0.99f)
            perp = QVector3D::crossProduct(axisNorm, QVector3D(0, 1, 0)).normalized();
        else
            perp = QVector3D::crossProduct(axisNorm, QVector3D(1, 0, 0)).normalized();

        const QVector3D biperp = QVector3D::crossProduct(axisNorm, perp).normalized();

        const float r = shaft.shaftRadius;
        const unsigned int baseIdx = static_cast<unsigned int>(vertices.size() / 6);

        // Generate circle vertices at tip and tail
        for (int ring = 0; ring < 2; ++ring) {
            const QVector3D center = (ring == 0) ? tipPos : tailPos;
            for (int i = 0; i < cylinderSides; ++i) {
                const float angle = 2.0f * float(M_PI) * float(i) / float(cylinderSides);
                const float cs = cosf(angle);
                const float sn = sinf(angle);

                const QVector3D normal = (perp * cs + biperp * sn).normalized();
                const QVector3D pos = center + normal * r;

                // position
                vertices.append(pos.x());
                vertices.append(pos.y());
                vertices.append(pos.z());
                // normal
                vertices.append(normal.x());
                vertices.append(normal.y());
                vertices.append(normal.z());
            }
        }

        // Side triangles (connect ring 0 to ring 1)
        for (int i = 0; i < cylinderSides; ++i) {
            const unsigned int i0 = baseIdx + static_cast<unsigned int>(i);
            const unsigned int i1 = baseIdx + static_cast<unsigned int>((i + 1) % cylinderSides);
            const unsigned int i2 = i0 + static_cast<unsigned int>(cylinderSides);
            const unsigned int i3 = i1 + static_cast<unsigned int>(cylinderSides);

            // Two triangles per quad
            indices.append(i0); indices.append(i2); indices.append(i1);
            indices.append(i1); indices.append(i2); indices.append(i3);
        }

        // Tip endcap (ring 0, center = tipPos)
        {
            const QVector3D normal = -axisNorm;
            const unsigned int centerIdx = static_cast<unsigned int>(vertices.size() / 6);
            vertices.append(tipPos.x());
            vertices.append(tipPos.y());
            vertices.append(tipPos.z());
            vertices.append(normal.x());
            vertices.append(normal.y());
            vertices.append(normal.z());

            for (int i = 0; i < cylinderSides; ++i) {
                const unsigned int i0 = baseIdx + static_cast<unsigned int>(i);
                const unsigned int i1 = baseIdx + static_cast<unsigned int>((i + 1) % cylinderSides);
                indices.append(centerIdx); indices.append(i1); indices.append(i0);
            }
        }

        // Tail endcap (ring 1, center = tailPos)
        {
            const QVector3D normal = axisNorm;
            const unsigned int centerIdx = static_cast<unsigned int>(vertices.size() / 6);
            vertices.append(tailPos.x());
            vertices.append(tailPos.y());
            vertices.append(tailPos.z());
            vertices.append(normal.x());
            vertices.append(normal.y());
            vertices.append(normal.z());

            const unsigned int ring1Base = baseIdx + static_cast<unsigned int>(cylinderSides);
            for (int i = 0; i < cylinderSides; ++i) {
                const unsigned int i0 = ring1Base + static_cast<unsigned int>(i);
                const unsigned int i1 = ring1Base + static_cast<unsigned int>((i + 1) % cylinderSides);
                indices.append(centerIdx); indices.append(i0); indices.append(i1);
            }
        }
    }
}

//=============================================================================================================

void ElectrodeObject::generateContactInstances(QVector<float>& instanceData) const
{
    instanceData.clear();
    const int floatsPerInstance = 9;
    instanceData.reserve(totalContactCount() * floatsPerInstance);

    for (const auto& shaft : m_shafts) {
        for (const auto& contact : shaft.contacts) {
            // position (3)
            instanceData.append(contact.position.x());
            instanceData.append(contact.position.y());
            instanceData.append(contact.position.z());
            // radius (1)
            instanceData.append(contact.radius);
            // color RGBA (4)
            instanceData.append(static_cast<float>(contact.color.redF()));
            instanceData.append(static_cast<float>(contact.color.greenF()));
            instanceData.append(static_cast<float>(contact.color.blueF()));
            instanceData.append(static_cast<float>(contact.color.alphaF()));
            // selected flag (1)
            instanceData.append(contact.selected ? 1.0f : 0.0f);
        }
    }
}

//=============================================================================================================

QVector3D ElectrodeObject::boundingBoxMin() const
{
    return m_bbMin;
}

//=============================================================================================================

QVector3D ElectrodeObject::boundingBoxMax() const
{
    return m_bbMax;
}

//=============================================================================================================

void ElectrodeObject::computeBoundingBox()
{
    m_bbMin = QVector3D(std::numeric_limits<float>::max(),
                        std::numeric_limits<float>::max(),
                        std::numeric_limits<float>::max());
    m_bbMax = QVector3D(std::numeric_limits<float>::lowest(),
                        std::numeric_limits<float>::lowest(),
                        std::numeric_limits<float>::lowest());

    for (const auto& shaft : m_shafts) {
        for (const auto& contact : shaft.contacts) {
            const float pad = contact.radius;
            const QVector3D& p = contact.position;

            if (p.x() - pad < m_bbMin.x()) m_bbMin.setX(p.x() - pad);
            if (p.y() - pad < m_bbMin.y()) m_bbMin.setY(p.y() - pad);
            if (p.z() - pad < m_bbMin.z()) m_bbMin.setZ(p.z() - pad);

            if (p.x() + pad > m_bbMax.x()) m_bbMax.setX(p.x() + pad);
            if (p.y() + pad > m_bbMax.y()) m_bbMax.setY(p.y() + pad);
            if (p.z() + pad > m_bbMax.z()) m_bbMax.setZ(p.z() + pad);
        }
    }
}

//=============================================================================================================

QColor ElectrodeObject::interpolateColor(float value, float minVal, float maxVal,
                                         const QColor& minColor, const QColor& maxColor)
{
    if (maxVal <= minVal)
        return minColor;

    float t = (value - minVal) / (maxVal - minVal);
    t = qBound(0.0f, t, 1.0f);

    const float r = static_cast<float>(minColor.redF())   * (1.0f - t) + static_cast<float>(maxColor.redF())   * t;
    const float g = static_cast<float>(minColor.greenF()) * (1.0f - t) + static_cast<float>(maxColor.greenF()) * t;
    const float b = static_cast<float>(minColor.blueF())  * (1.0f - t) + static_cast<float>(maxColor.blueF())  * t;
    const float a = static_cast<float>(minColor.alphaF()) * (1.0f - t) + static_cast<float>(maxColor.alphaF()) * t;

    QColor result;
    result.setRedF(static_cast<qreal>(r));
    result.setGreenF(static_cast<qreal>(g));
    result.setBlueF(static_cast<qreal>(b));
    result.setAlphaF(static_cast<qreal>(a));
    return result;
}
