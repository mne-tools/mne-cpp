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

#include <mne/disp/plots/helpers/colormap.h>
#include <mne/ieeg/ieeg_surface_map.h>
#include <mne/scene/ray_picking.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMatrix4x4>
#include <QQuaternion>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <format>
#include <limits>
#include <map>
#include <numbers>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne::disp;

namespace
{

struct MeshVertex
{
    float x, y, z, nx, ny, nz;
};

// Unit icosphere, subdivided twice (320 triangles)
void unitSphere(std::vector<std::byte>& vertices, std::vector<std::byte>& indices, int& indexCount)
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
    mv.reserve(v.size());
    for (const auto& p : v)
        mv.push_back({p.x(), p.y(), p.z(), p.x(), p.y(), p.z()});
    vertices = toByteBuffer(mv);
    indices = toByteBuffer(idx);
    indexCount = int(idx.size());
}

// Unit cylinder along +y from -0.5 to 0.5, with caps
void unitCylinder(std::vector<std::byte>& vertices, std::vector<std::byte>& indices, int& indexCount)
{
    constexpr int kSides = 12;
    std::vector<MeshVertex> mv;
    std::vector<uint32_t> idx;
    for (int i = 0; i < kSides; ++i) {
        const float a = 2.0f * float(std::numbers::pi) * float(i) / float(kSides);
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
            const float a = 2.0f * float(std::numbers::pi) * float(i) / float(kSides);
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
    vertices = toByteBuffer(mv);
    indices = toByteBuffer(idx);
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

void ElectrodeObject::setImplantation(const mne::IeegImplantation& implantation)
{
    m_implantation = implantation;
    const int contactCount = int(m_implantation.contacts().size());
    m_leadOfContact.assign(contactCount, -1);
    const auto& leads = m_implantation.leads();
    for (int l = 0; l < int(leads.size()); ++l) {
        for (const int c : leads[l].contacts)
            m_leadOfContact[c] = l;
    }
    m_contactColors.assign(contactCount, QColor());
    m_contactValues.clear();
    m_selectedContact = -1;
    m_hoveredContact = -1;
    computeBoundingBox();
    buildBodies();
    buildContactInstances();
}

//=============================================================================================================

int ElectrodeObject::leadOf(int contact) const
{
    return contact >= 0 && contact < std::ssize(m_leadOfContact) ? m_leadOfContact[static_cast<std::size_t>(contact)] : -1;
}

//=============================================================================================================

int ElectrodeObject::totalContactCount() const
{
    return int(m_implantation.contacts().size());
}

//=============================================================================================================

void ElectrodeObject::setContactValues(const std::map<std::string, float>& values, const std::string& colormap)
{
    if (values.empty()) {
        clearContactValues();
        return;
    }
    m_contactValues = values;
    m_colormap = colormap;
    m_valueMin = 0.0f;
    m_valueMax = std::numeric_limits<float>::lowest();
    for (const auto& [name, v] : values) {
        m_valueMin = std::min(m_valueMin, v);
        m_valueMax = std::max(m_valueMax, v);
    }

    const auto& contacts = m_implantation.contacts();
    std::ranges::fill(m_contactColors, QColor());
    const QString colormapName = QString::fromStdString(colormap);
    for (int c = 0; c < int(contacts.size()); ++c) {
        const auto it = values.find(contacts[c].name);
        if (it != values.end())
            m_contactColors[c] = QColor::fromRgb(mne::disp::ColorMap::valueToColor(normalized(it->second), colormapName));
    }
    buildContactInstances();
}

//=============================================================================================================

void ElectrodeObject::clearContactValues()
{
    m_contactValues.clear();
    std::ranges::fill(m_contactColors, QColor());
    buildContactInstances();
}

//=============================================================================================================

float ElectrodeObject::normalized(float value) const
{
    return m_valueMax > m_valueMin ? std::clamp((value - m_valueMin) / (m_valueMax - m_valueMin), 0.0f, 1.0f) : 1.0f;
}

//=============================================================================================================

std::vector<uint32_t> ElectrodeObject::surfaceColors(const Eigen::MatrixX3f& vertices, double distance) const
{
    if (m_contactValues.empty() || vertices.rows() == 0)
        return {};

    // Weights (vertices x contacts) of ECoG contacts moved onto the surface plus depth contacts in place
    const auto& contacts = m_implantation.contacts();
    const auto& leads = m_implantation.leads();
    const Eigen::Index n = contacts.size();
    Eigen::MatrixX3f subdural = Eigen::MatrixX3f::Constant(n, 3, std::numeric_limits<float>::quiet_NaN());
    Eigen::MatrixX3f depth = subdural;
    Eigen::VectorXd value = Eigen::VectorXd::Zero(n);
    Eigen::VectorXd hasValue = Eigen::VectorXd::Zero(n);
    for (int c = 0; c < n; ++c) {
        const auto it = m_contactValues.find(contacts[c].name);
        if (it == m_contactValues.end())
            continue;
        const int lead = leadOf(c);
        const bool isDepth = lead < 0 || leads[lead].type == mne::IeegLeadType::Depth;
        (isDepth ? depth : subdural).row(c) = contacts[c].position.transpose();
        value(c) = it->second;
        hasValue(c) = 1.0;
    }
    const Eigen::SparseMatrix<double> weights = mne::IeegSurfaceMap::weights(subdural, vertices, distance, mne::IeegSurfaceMapMode::Sum, true) +
        mne::IeegSurfaceMap::weights(depth, vertices, distance, mne::IeegSurfaceMapMode::Sum, false);
    const Eigen::VectorXd weighted = weights * value;
    const Eigen::VectorXd reach = weights * hasValue;

    std::vector<uint32_t> colors(static_cast<std::size_t>(vertices.rows()), packABGR(0, 0, 0, 0));
    const QString colormapName = QString::fromStdString(m_colormap);
    for (int v = 0; v < std::ssize(colors); ++v) {
        if (reach(v) <= 0.0)
            continue;
        const QRgb rgb = mne::disp::ColorMap::valueToColor(normalized(static_cast<float>(weighted(v) / reach(v))), colormapName);
        colors[v] = packABGR(qRed(rgb), qGreen(rgb), qBlue(rgb), static_cast<uint32_t>(255.0 * std::min(1.0, reach(v))));
    }
    return colors;
}

//=============================================================================================================

void ElectrodeObject::selectContact(const std::string& name)
{
    m_selectedContact = name.empty() ? -1 : m_implantation.index_of(name);
    buildContactInstances();
}

//=============================================================================================================

void ElectrodeObject::clearSelection()
{
    selectContact(std::string());
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

std::string ElectrodeObject::selectedContact() const
{
    return m_selectedContact >= 0 ? m_implantation.contacts()[m_selectedContact].name : std::string();
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
    const int lead = leadOf(contact);
    const bool depth = lead < 0 || m_implantation.leads()[lead].type == mne::IeegLeadType::Depth;
    return depth ? kDepthContactRadius : kSubduralContactRadius;
}

//=============================================================================================================

void ElectrodeObject::buildContactInstances()
{
    const auto& contacts = m_implantation.contacts();
    std::vector<InstanceData> instances;
    instances.reserve(size_t(contacts.size()));
    for (int c = 0; c < int(contacts.size()); ++c) {
        const Eigen::Vector3f& p = contacts[c].position;
        QMatrix4x4 model;
        model.translate(p.x(), p.y(), p.z());
        model.scale(contactRadius(c));
        const QColor color = (c < std::ssize(m_contactColors) && m_contactColors[c].isValid()) ? m_contactColors[c] : leadColor(std::max(0, leadOf(c)));
        InstanceData d;
        std::memcpy(d.model, model.constData(), sizeof(d.model));
        d.color[0] = float(color.redF());
        d.color[1] = float(color.greenF());
        d.color[2] = float(color.blueF());
        d.color[3] = float(color.alphaF());
        d.isSelected = (c == m_selectedContact || c == m_hoveredContact) ? 1.0f : 0.0f;
        instances.push_back(d);
    }
    m_contactInstances = toByteBuffer(instances);
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
    const auto& contacts = m_implantation.contacts();
    auto pos = [&](int c) {
        const Eigen::Vector3f& p = contacts[c].position;
        return QVector3D(p.x(), p.y(), p.z());
    };

    const auto& leads = m_implantation.leads();
    for (int l = 0; l < int(leads.size()); ++l) {
        const mne::IeegLead& lead = leads[l];
        const auto& order = lead.contacts;
        if (order.size() < 2)
            continue;
        // The body is darker than the contacts so these stand out along it
        const QColor body = leadColor(l).darker(150);
        if (lead.type == mne::IeegLeadType::Depth) {
            const QVector3D tip = pos(order.front());
            const QVector3D tail = pos(order.back());
            const QVector3D dir = (tail - tip).normalized();
            addCylinder(tip - dir * kShaftOvershoot, tail + dir * kShaftOvershoot, kShaftRadius, body);
            continue;
        }
        // Strips and grids: ties between neighbours along rows, and down columns for a grid of known shape
        const int cols = (lead.type == mne::IeegLeadType::Grid && lead.cols > 0) ? lead.cols : int(order.size());
        auto tie = [&](int i, int j) {
            if ((pos(order[i]) - pos(order[j])).length() <= kMaxTieLength)
                addCylinder(pos(order[i]), pos(order[j]), kTieRadius, body);
        };
        for (int i = 0; i < int(order.size()); ++i) {
            if ((i + 1) % cols != 0 && i + 1 < int(order.size()))
                tie(i, i + 1);
            if (i + cols < int(order.size()))
                tie(i, i + cols);
        }
    }
    m_bodyInstances = toByteBuffer(instances);
    m_bodyCount = int(instances.size());
    ++m_bodyGeneration;
}

//=============================================================================================================

int ElectrodeObject::intersect(const QVector3D& rayOrigin, const QVector3D& rayDir, float& dist) const
{
    int best = -1;
    float bestT = std::numeric_limits<float>::max();
    const auto& contacts = m_implantation.contacts();
    const scene::Ray ray{{rayOrigin.x(), rayOrigin.y(), rayOrigin.z()}, {rayDir.x(), rayDir.y(), rayDir.z()}};
    for (int c = 0; c < int(contacts.size()); ++c) {
        const float r = contactRadius(c);
        const Eigen::Vector3f& p = contacts[c].position;
        const std::optional<scene::SphereCrossing> crossing = scene::intersect_ray_sphere(ray, p, r);
        if (crossing && crossing->entry > 0.0f && crossing->entry < bestT) {
            bestT = crossing->entry;
            best = c;
        }
    }
    if (best >= 0)
        dist = bestT;
    return best;
}

//=============================================================================================================

const mne::IeegContact* ElectrodeObject::contactAt(int index) const
{
    return (index >= 0 && index < int(m_implantation.contacts().size())) ? &m_implantation.contacts()[index] : nullptr;
}

//=============================================================================================================

std::string ElectrodeObject::contactLabel(int index) const
{
    const mne::IeegContact* contact = contactAt(index);
    const int lead = leadOf(index);
    if (!contact || lead < 0)
        return std::string();
    const mne::IeegLead& l = m_implantation.leads()[lead];
    const std::string_view kind = l.type == mne::IeegLeadType::Depth ? "depth lead"
        : l.type == mne::IeegLeadType::Strip                         ? "strip"
                                                                     : "grid";
    return std::format("{} ({} {})", contact->name, kind, l.name);
}

//=============================================================================================================

InstancedStreams ElectrodeObject::contactStreams() const
{
    return {{m_sphereVertices.data(), quint32(m_sphereVertices.size()), 0},
            {m_sphereIndices.data(), quint32(m_sphereIndices.size()), 0},
            {m_contactInstances.data(), quint32(m_contactInstances.size()), m_contactGeneration},
            m_sphereIndexCount,
            totalContactCount()};
}

//=============================================================================================================

InstancedStreams ElectrodeObject::bodyStreams() const
{
    return {{m_cylinderVertices.data(), quint32(m_cylinderVertices.size()), 0},
            {m_cylinderIndices.data(), quint32(m_cylinderIndices.size()), 0},
            {m_bodyInstances.data(), quint32(m_bodyInstances.size()), m_bodyGeneration},
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
    const auto& contacts = m_implantation.contacts();
    for (int c = 0; c < int(contacts.size()); ++c) {
        const float pad = contactRadius(c);
        const QVector3D p(contacts[c].position.x(), contacts[c].position.y(), contacts[c].position.z());
        m_bbMin = QVector3D(std::min(m_bbMin.x(), p.x() - pad), std::min(m_bbMin.y(), p.y() - pad), std::min(m_bbMin.z(), p.z() - pad));
        m_bbMax = QVector3D(std::max(m_bbMax.x(), p.x() + pad), std::max(m_bbMax.y(), p.y() + pad), std::max(m_bbMax.z(), p.z() + pad));
    }
}
