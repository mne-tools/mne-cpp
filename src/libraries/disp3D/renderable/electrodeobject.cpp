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

void ElectrodeObject::setImplantation(const IEEGLIB::IeegImplantation& implantation)
{
    m_implantation = implantation;
    const int contactCount = int(m_implantation.contacts().size());
    m_leadOfContact.fill(-1, contactCount);
    const QVector<IEEGLIB::IeegLead>& leads = m_implantation.leads();
    for (int l = 0; l < leads.size(); ++l) {
        for (const int c : leads[l].contacts)
            m_leadOfContact[c] = l;
    }
    m_contactColors.fill(QColor(), contactCount);
    m_selectedContact = -1;
    m_hoveredContact = -1;
    computeBoundingBox();
    buildBodies();
    buildContactInstances();
}

//=============================================================================================================

int ElectrodeObject::totalContactCount() const
{
    return int(m_implantation.contacts().size());
}

//=============================================================================================================

void ElectrodeObject::setContactValues(const QMap<QString, float>& values,
                                       const QColor& minColor,
                                       const QColor& maxColor)
{
    if (values.isEmpty())
        return;

    float minVal = std::numeric_limits<float>::max();
    float maxVal = std::numeric_limits<float>::lowest();
    for (auto it = values.cbegin(); it != values.cend(); ++it) {
        minVal = std::min(minVal, it.value());
        maxVal = std::max(maxVal, it.value());
    }

    const QVector<IEEGLIB::IeegContact>& contacts = m_implantation.contacts();
    for (int c = 0; c < contacts.size(); ++c) {
        const auto it = values.find(contacts[c].name);
        if (it != values.end())
            m_contactColors[c] = interpolateColor(it.value(), minVal, maxVal, minColor, maxColor);
    }
    buildContactInstances();
}

//=============================================================================================================

void ElectrodeObject::clearContactValues()
{
    m_contactColors.fill(QColor());
    buildContactInstances();
}

//=============================================================================================================

void ElectrodeObject::selectContact(const QString& name)
{
    m_selectedContact = name.isEmpty() ? -1 : m_implantation.indexOf(name);
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
    return m_selectedContact >= 0 ? m_implantation.contacts()[m_selectedContact].name : QString();
}

//=============================================================================================================

QColor ElectrodeObject::leadColor(int index)
{
    const double hue = std::fmod(double(index) * 0.618033988749895, 1.0);
    return QColor::fromHslF(float(hue), 0.72f, 0.62f);
}

//=============================================================================================================

float ElectrodeObject::contactRadius(int contact) const
{
    const int lead = m_leadOfContact.value(contact, -1);
    const bool depth = lead < 0 || m_implantation.leads()[lead].type == IEEGLIB::IeegLeadType::Depth;
    return depth ? kDepthContactRadius : kSubduralContactRadius;
}

//=============================================================================================================

void ElectrodeObject::buildContactInstances()
{
    const QVector<IEEGLIB::IeegContact>& contacts = m_implantation.contacts();
    std::vector<InstanceData> instances;
    instances.reserve(size_t(contacts.size()));
    for (int c = 0; c < contacts.size(); ++c) {
        const Eigen::Vector3f& p = contacts[c].position;
        QMatrix4x4 model;
        model.translate(p.x(), p.y(), p.z());
        model.scale(contactRadius(c));
        const QColor color = m_contactColors.value(c).isValid() ? m_contactColors[c] : leadColor(std::max(0, m_leadOfContact.value(c, 0)));
        InstanceData d;
        std::memcpy(d.model, model.constData(), sizeof(d.model));
        d.color[0] = float(color.redF());
        d.color[1] = float(color.greenF());
        d.color[2] = float(color.blueF());
        d.color[3] = float(color.alphaF());
        d.isSelected = (c == m_selectedContact || c == m_hoveredContact) ? 1.0f : 0.0f;
        instances.push_back(d);
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
    const QVector<IEEGLIB::IeegContact>& contacts = m_implantation.contacts();
    auto pos = [&](int c) {
        const Eigen::Vector3f& p = contacts[c].position;
        return QVector3D(p.x(), p.y(), p.z());
    };

    const QVector<IEEGLIB::IeegLead>& leads = m_implantation.leads();
    for (int l = 0; l < leads.size(); ++l) {
        const IEEGLIB::IeegLead& lead = leads[l];
        const QVector<int>& order = lead.contacts;
        if (order.size() < 2)
            continue;
        // The body is darker than the contacts so these stand out along it
        const QColor body = leadColor(l).darker(150);
        if (lead.type == IEEGLIB::IeegLeadType::Depth) {
            const QVector3D tip = pos(order.first());
            const QVector3D tail = pos(order.last());
            const QVector3D dir = (tail - tip).normalized();
            addCylinder(tip - dir * kShaftOvershoot, tail + dir * kShaftOvershoot, kShaftRadius, body);
            continue;
        }
        // Strips and grids: ties between neighbours along rows, and down columns for a grid of known shape
        const int cols = (lead.type == IEEGLIB::IeegLeadType::Grid && lead.cols > 0) ? lead.cols : int(order.size());
        auto tie = [&](int i, int j) {
            if ((pos(order[i]) - pos(order[j])).length() <= kMaxTieLength)
                addCylinder(pos(order[i]), pos(order[j]), kTieRadius, body);
        };
        for (int i = 0; i < order.size(); ++i) {
            if ((i + 1) % cols != 0 && i + 1 < order.size())
                tie(i, i + 1);
            if (i + cols < order.size())
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
    const QVector<IEEGLIB::IeegContact>& contacts = m_implantation.contacts();
    for (int c = 0; c < contacts.size(); ++c) {
        const float r = contactRadius(c);
        const Eigen::Vector3f& p = contacts[c].position;
        const QVector3D oc = rayOrigin - QVector3D(p.x(), p.y(), p.z());
        const float b = QVector3D::dotProduct(oc, rayDir);
        const float disc = b * b - (oc.lengthSquared() - r * r);
        if (disc >= 0.0f) {
            const float t = -b - std::sqrt(disc);
            if (t > 0.0f && t < bestT) {
                bestT = t;
                best = c;
            }
        }
    }
    if (best >= 0)
        dist = bestT;
    return best;
}

//=============================================================================================================

const IEEGLIB::IeegContact* ElectrodeObject::contactAt(int index) const
{
    return (index >= 0 && index < m_implantation.contacts().size()) ? &m_implantation.contacts()[index] : nullptr;
}

//=============================================================================================================

QString ElectrodeObject::contactLabel(int index) const
{
    const IEEGLIB::IeegContact* contact = contactAt(index);
    const int lead = m_leadOfContact.value(index, -1);
    if (!contact || lead < 0)
        return QString();
    const IEEGLIB::IeegLead& l = m_implantation.leads()[lead];
    const QString kind = l.type == IEEGLIB::IeegLeadType::Depth ? QStringLiteral("depth lead")
        : l.type == IEEGLIB::IeegLeadType::Strip                ? QStringLiteral("strip")
                                                                : QStringLiteral("grid");
    return QStringLiteral("%1 (%2 %3)").arg(contact->name, kind, l.name);
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
    m_bbMin = QVector3D(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    m_bbMax = QVector3D(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());
    const QVector<IEEGLIB::IeegContact>& contacts = m_implantation.contacts();
    for (int c = 0; c < contacts.size(); ++c) {
        const float pad = contactRadius(c);
        const QVector3D p(contacts[c].position.x(), contacts[c].position.y(), contacts[c].position.z());
        m_bbMin = QVector3D(std::min(m_bbMin.x(), p.x() - pad), std::min(m_bbMin.y(), p.y() - pad), std::min(m_bbMin.z(), p.z() - pad));
        m_bbMax = QVector3D(std::max(m_bbMax.x(), p.x() + pad), std::max(m_bbMax.y(), p.y() + pad), std::max(m_bbMax.z(), p.z() + pad));
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
