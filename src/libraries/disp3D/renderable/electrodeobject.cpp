//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     electrodeobject.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Cylinder-shaft and contact-sphere geometry generation plus contact-value colour mapping for sEEG / ECoG arrays.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "electrodeobject.h"

#include <QMatrix4x4>
#include <QQuaternion>
#include <QtMath>

#include <array>
#include <cstring>
#include <limits>
#include <map>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;

namespace
{

struct MeshVertex
{
    float x, y, z, nx, ny, nz;
};

template<typename T>
QByteArray toBytes(const std::vector<T>& v)
{
    return QByteArray(reinterpret_cast<const char*>(v.data()), qsizetype(v.size() * sizeof(T)));
}

// Unit icosphere, subdivided twice (320 triangles)
void unitSphere(QByteArray& vertices, QByteArray& indices, int& indexCount)
{
    const float t = (1.0f + std::sqrt(5.0f)) / 2.0f;
    std::vector<QVector3D> v = {{-1, t, 0}, {1, t, 0}, {-1, -t, 0}, {1, -t, 0}, {0, -1, t}, {0, 1, t}, {0, -1, -t}, {0, 1, -t}, {t, 0, -1}, {t, 0, 1}, {-t, 0, -1}, {-t, 0, 1}};
    for (auto& p : v)
        p.normalize();
    std::vector<uint32_t> idx = {0, 11, 5, 0, 5, 1, 0, 1, 7, 0, 7, 10, 0, 10, 11, 1, 5, 9, 5, 11, 4, 11, 10, 2, 10, 7, 6, 7, 1, 8,
                                 3, 9, 4, 3, 4, 2, 3, 2, 6, 3, 6, 8, 3, 8, 9, 4, 9, 5, 2, 4, 11, 6, 2, 10, 8, 6, 7, 9, 8, 1};
    for (int level = 0; level < 2; ++level) {
        std::map<std::pair<uint32_t, uint32_t>, uint32_t> midpoints;
        auto midpoint = [&](uint32_t a, uint32_t b) {
            const auto key = std::minmax(a, b);
            const auto it = midpoints.find(key);
            if (it != midpoints.end())
                return it->second;
            v.push_back(((v[a] + v[b]) * 0.5f).normalized());
            return midpoints[key] = uint32_t(v.size() - 1);
        };
        std::vector<uint32_t> next;
        for (size_t i = 0; i < idx.size(); i += 3) {
            const uint32_t a = idx[i], b = idx[i + 1], c = idx[i + 2];
            const uint32_t ab = midpoint(a, b), bc = midpoint(b, c), ca = midpoint(c, a);
            next.insert(next.end(), {a, ab, ca, b, bc, ab, c, ca, bc, ab, bc, ca});
        }
        idx = std::move(next);
    }
    std::vector<MeshVertex> mv;
    for (const auto& p : v)
        mv.push_back({p.x(), p.y(), p.z(), p.x(), p.y(), p.z()});
    vertices = toBytes(mv);
    indices = toBytes(idx);
    indexCount = int(idx.size());
}

// Unit cylinder along +y from -0.5 to 0.5, with caps
void unitCylinder(QByteArray& vertices, QByteArray& indices, int& indexCount)
{
    constexpr int kSides = 12;
    std::vector<MeshVertex> mv;
    std::vector<uint32_t> idx;
    for (int i = 0; i < kSides; ++i) {
        const float a = 2.0f * float(M_PI) * float(i) / float(kSides);
        const float x = std::cos(a), z = std::sin(a);
        mv.push_back({x, 0.5f, z, x, 0, z});
        mv.push_back({x, -0.5f, z, x, 0, z});
    }
    for (int i = 0; i < kSides; ++i) {
        const uint32_t top = uint32_t(2 * i), bottom = top + 1;
        const uint32_t nextTop = uint32_t(2 * ((i + 1) % kSides)), nextBottom = nextTop + 1;
        idx.insert(idx.end(), {top, nextTop, bottom, bottom, nextTop, nextBottom});
    }
    for (const float y : {0.5f, -0.5f}) {
        const uint32_t centre = uint32_t(mv.size());
        mv.push_back({0, y, 0, 0, y * 2.0f, 0});
        for (int i = 0; i < kSides; ++i) {
            const float a = 2.0f * float(M_PI) * float(i) / float(kSides);
            mv.push_back({std::cos(a), y, std::sin(a), 0, y * 2.0f, 0});
        }
        for (int i = 0; i < kSides; ++i) {
            const uint32_t cur = centre + 1 + uint32_t(i), next = centre + 1 + uint32_t((i + 1) % kSides);
            if (y > 0)
                idx.insert(idx.end(), {centre, next, cur});
            else
                idx.insert(idx.end(), {centre, cur, next});
        }
    }
    vertices = toBytes(mv);
    indices = toBytes(idx);
    indexCount = int(idx.size());
}

QColor arrayColor(const ElectrodeArray& array, int arrayIndex)
{
    return array.shaftColor.isValid() ? array.shaftColor : ElectrodeObject::leadColor(arrayIndex);
}

} // namespace

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
    unitSphere(m_sphereVertices, m_sphereIndices, m_sphereIndexCount);
    unitCylinder(m_cylinderVertices, m_cylinderIndices, m_cylinderIndexCount);
}

//=============================================================================================================

ElectrodeObject::~ElectrodeObject() = default;

//=============================================================================================================

void ElectrodeObject::setArrays(const QVector<ElectrodeArray>& arrays)
{
    m_arrays = arrays;
    m_selectedContact.clear();
    m_hoveredContact = -1;
    for (auto& array : m_arrays) {
        for (auto& contact : array.contacts)
            contact.selected = false;
    }
    computeBoundingBox();
    buildBodies();
    buildContactInstances();
}

//=============================================================================================================

const QVector<ElectrodeArray>& ElectrodeObject::arrays() const
{
    return m_arrays;
}

//=============================================================================================================

int ElectrodeObject::totalContactCount() const
{
    int count = 0;
    for (const auto& shaft : m_arrays)
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
        if (it.value() < minVal)
            minVal = it.value();
        if (it.value() > maxVal)
            maxVal = it.value();
    }

    // Apply to contacts
    for (auto& shaft : m_arrays) {
        for (auto& contact : shaft.contacts) {
            auto it = values.find(contact.name);
            if (it != values.end()) {
                contact.value = it.value();
                contact.color = interpolateColor(it.value(), minVal, maxVal,
                                                 minColor, maxColor);
            }
        }
    }
    buildContactInstances();
}

//=============================================================================================================

void ElectrodeObject::selectContact(const QString& name)
{
    m_selectedContact.clear();
    for (auto& array : m_arrays) {
        for (auto& contact : array.contacts) {
            contact.selected = !name.isEmpty() && contact.name == name && m_selectedContact.isEmpty();
            if (contact.selected)
                m_selectedContact = name;
        }
    }
    buildContactInstances();
}

//=============================================================================================================

void ElectrodeObject::clearSelection()
{
    selectContact(QString());
}

//=============================================================================================================

void ElectrodeObject::setHoveredContact(int index)
{
    if (index == m_hoveredContact)
        return;
    m_hoveredContact = index;
    buildContactInstances();
}

//=============================================================================================================

QString ElectrodeObject::selectedContact() const
{
    return m_selectedContact;
}

//=============================================================================================================

QColor ElectrodeObject::leadColor(int index)
{
    const double hue = std::fmod(double(index) * 0.618033988749895, 1.0);
    return QColor::fromHslF(float(hue), 0.72f, 0.62f);
}

//=============================================================================================================

float ElectrodeObject::contactRadius(const ElectrodeArray& array, const ElectrodeContact& contact) const
{
    if (contact.radius > 0.0f)
        return contact.radius;
    return array.layout == ElectrodeLayout::Depth ? kDepthContactRadius : kSubduralContactRadius;
}

//=============================================================================================================

void ElectrodeObject::buildContactInstances()
{
    std::vector<InstanceData> instances;
    instances.reserve(size_t(totalContactCount()));
    int index = 0;
    for (int a = 0; a < m_arrays.size(); ++a) {
        const ElectrodeArray& array = m_arrays[a];
        const QColor lead = arrayColor(array, a);
        for (const ElectrodeContact& contact : array.contacts) {
            QMatrix4x4 model;
            model.translate(contact.position);
            model.scale(contactRadius(array, contact));
            const QColor c = contact.color.isValid() ? contact.color : lead;
            InstanceData d;
            std::memcpy(d.model, model.constData(), sizeof(d.model));
            d.color[0] = float(c.redF());
            d.color[1] = float(c.greenF());
            d.color[2] = float(c.blueF());
            d.color[3] = float(c.alphaF());
            d.isSelected = (contact.selected || index == m_hoveredContact) ? 1.0f : 0.0f;
            instances.push_back(d);
            ++index;
        }
    }
    m_contactInstances = toBytes(instances);
    ++m_contactGeneration;
}

//=============================================================================================================

void ElectrodeObject::buildBodies()
{
    std::vector<InstanceData> instances;
    auto addCylinder = [&](const QVector3D& from, const QVector3D& to, float radius, const QColor& color) {
        const QVector3D axis = to - from;
        const float length = axis.length();
        if (length < 1e-6f)
            return;
        QMatrix4x4 model;
        model.translate((from + to) * 0.5f);
        model.rotate(QQuaternion::rotationTo(QVector3D(0.0f, 1.0f, 0.0f), axis / length));
        model.scale(radius, length, radius);
        InstanceData d;
        std::memcpy(d.model, model.constData(), sizeof(d.model));
        d.color[0] = float(color.redF());
        d.color[1] = float(color.greenF());
        d.color[2] = float(color.blueF());
        d.color[3] = 1.0f;
        d.isSelected = 0.0f;
        instances.push_back(d);
    };

    for (int a = 0; a < m_arrays.size(); ++a) {
        const ElectrodeArray& array = m_arrays[a];
        const QVector<ElectrodeContact>& contacts = array.contacts;
        if (contacts.size() < 2)
            continue;
        // The body is darker than the contacts so these stand out along it
        const QColor body = arrayColor(array, a).darker(150);
        if (array.layout == ElectrodeLayout::Depth) {
            const QVector3D tip = contacts.first().position;
            const QVector3D tail = contacts.last().position;
            const QVector3D dir = (tail - tip).normalized();
            addCylinder(tip - dir * kShaftOvershoot, tail + dir * kShaftOvershoot,
                        array.shaftRadius > 0.0f ? array.shaftRadius : kShaftRadius, body);
            continue;
        }
        // Strips and grids: ties between neighbours along rows, and along columns for a declared grid
        const int cols = (array.layout == ElectrodeLayout::Grid && array.gridRows * array.gridCols == contacts.size())
            ? array.gridCols
            : int(contacts.size());
        auto tie = [&](int i, int j) {
            if ((contacts[i].position - contacts[j].position).length() <= kMaxTieLength)
                addCylinder(contacts[i].position, contacts[j].position, kTieRadius, body);
        };
        for (int i = 0; i < contacts.size(); ++i) {
            if ((i + 1) % cols != 0 && i + 1 < contacts.size())
                tie(i, i + 1);
            if (i + cols < contacts.size())
                tie(i, i + cols);
        }
    }
    m_bodyInstances = toBytes(instances);
    m_bodyCount = int(instances.size());
    ++m_bodyGeneration;
}

//=============================================================================================================

int ElectrodeObject::intersect(const QVector3D& rayOrigin, const QVector3D& rayDir, float& dist) const
{
    int best = -1;
    float bestT = std::numeric_limits<float>::max();
    int index = 0;
    for (const ElectrodeArray& array : m_arrays) {
        for (const ElectrodeContact& contact : array.contacts) {
            const float r = contactRadius(array, contact);
            const QVector3D oc = rayOrigin - contact.position;
            const float b = QVector3D::dotProduct(oc, rayDir);
            const float disc = b * b - (oc.lengthSquared() - r * r);
            if (disc >= 0.0f) {
                const float t = -b - std::sqrt(disc);
                if (t > 0.0f && t < bestT) {
                    bestT = t;
                    best = index;
                }
            }
            ++index;
        }
    }
    if (best >= 0)
        dist = bestT;
    return best;
}

//=============================================================================================================

const ElectrodeContact* ElectrodeObject::contactAt(int index) const
{
    if (index < 0)
        return nullptr;
    for (const ElectrodeArray& array : m_arrays) {
        if (index < array.contacts.size())
            return &array.contacts[index];
        index -= int(array.contacts.size());
    }
    return nullptr;
}

//=============================================================================================================

QString ElectrodeObject::contactLabel(int index) const
{
    if (index < 0)
        return QString();
    for (const ElectrodeArray& array : m_arrays) {
        if (index < array.contacts.size()) {
            const QString kind = array.layout == ElectrodeLayout::Depth ? QStringLiteral("depth lead")
                : array.layout == ElectrodeLayout::Strip                ? QStringLiteral("strip")
                                                                        : QStringLiteral("grid");
            return QStringLiteral("%1 (%2 %3)").arg(array.contacts[index].name, kind, array.label);
        }
        index -= int(array.contacts.size());
    }
    return QString();
}

//=============================================================================================================

InstancedStreams ElectrodeObject::contactStreams() const
{
    return {{m_sphereVertices.constData(), quint32(m_sphereVertices.size()), 0},
            {m_sphereIndices.constData(), quint32(m_sphereIndices.size()), 0},
            {m_contactInstances.constData(), quint32(m_contactInstances.size()), m_contactGeneration},
            m_sphereIndexCount,
            totalContactCount()};
}

//=============================================================================================================

InstancedStreams ElectrodeObject::bodyStreams() const
{
    return {{m_cylinderVertices.constData(), quint32(m_cylinderVertices.size()), 0},
            {m_cylinderIndices.constData(), quint32(m_cylinderIndices.size()), 0},
            {m_bodyInstances.constData(), quint32(m_bodyInstances.size()), m_bodyGeneration},
            m_cylinderIndexCount,
            m_bodyCount};
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

    for (const auto& shaft : m_arrays) {
        for (const auto& contact : shaft.contacts) {
            const float pad = contactRadius(shaft, contact);
            const QVector3D& p = contact.position;

            if (p.x() - pad < m_bbMin.x())
                m_bbMin.setX(p.x() - pad);
            if (p.y() - pad < m_bbMin.y())
                m_bbMin.setY(p.y() - pad);
            if (p.z() - pad < m_bbMin.z())
                m_bbMin.setZ(p.z() - pad);

            if (p.x() + pad > m_bbMax.x())
                m_bbMax.setX(p.x() + pad);
            if (p.y() + pad > m_bbMax.y())
                m_bbMax.setY(p.y() + pad);
            if (p.z() + pad > m_bbMax.z())
                m_bbMax.setZ(p.z() + pad);
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

    const float r = static_cast<float>(minColor.redF()) * (1.0f - t) + static_cast<float>(maxColor.redF()) * t;
    const float g = static_cast<float>(minColor.greenF()) * (1.0f - t) + static_cast<float>(maxColor.greenF()) * t;
    const float b = static_cast<float>(minColor.blueF()) * (1.0f - t) + static_cast<float>(maxColor.blueF()) * t;
    const float a = static_cast<float>(minColor.alphaF()) * (1.0f - t) + static_cast<float>(maxColor.alphaF()) * t;

    QColor result;
    result.setRedF(static_cast<qreal>(r));
    result.setGreenF(static_cast<qreal>(g));
    result.setBlueF(static_cast<qreal>(b));
    result.setAlphaF(static_cast<qreal>(a));
    return result;
}
