//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     ieeg_implantation.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Lead grouping, ordering and typing of sEEG / ECoG contacts, and BIDS iEEG reading.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ieeg_implantation.h"

#include <bids/bids_coordinate_system.h>
#include <bids/bids_electrode.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QRegularExpression>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <limits>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace IEEGLIB;

namespace
{

// "LA10" -> ("LA", 10); "A'2" -> ("A'", 2); a name without a trailing number has no number
QRegularExpressionMatch splitName(const QString& name)
{
    static const QRegularExpression trailingNumber(QStringLiteral("^(.*?)[\\s_-]*(\\d+)$"));
    return trailingNumber.match(name);
}

void setError(QString* error, const QString& message)
{
    if (error)
        *error = message;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

IeegImplantation::IeegImplantation(const QVector<IeegContact>& contacts, const QString& coordinateSystem)
: m_contacts(contacts)
, m_coordinateSystem(coordinateSystem)
{
    QHash<QString, int> leadIndex;
    QVector<QVector<QPair<int, int>>> members; // per lead: (number along the lead, contact index)
    for (int i = 0; i < m_contacts.size(); ++i) {
        IeegContact& contact = m_contacts[i];
        const QRegularExpressionMatch m = splitName(contact.name);
        if (contact.lead.isEmpty())
            contact.lead = (m.hasMatch() && !m.captured(1).isEmpty()) ? m.captured(1) : contact.name;
        if (!leadIndex.contains(contact.lead)) {
            leadIndex.insert(contact.lead, int(m_leads.size()));
            IeegLead lead;
            lead.name = contact.lead;
            m_leads.append(lead);
            members.append(QVector<QPair<int, int>>());
        }
        members[leadIndex.value(contact.lead)].append({m.hasMatch() ? m.captured(2).toInt() : 0, i});
    }

    for (int l = 0; l < m_leads.size(); ++l) {
        QVector<QPair<int, int>>& lead = members[l];
        std::stable_sort(lead.begin(), lead.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
        float spacing = std::numeric_limits<float>::max();
        for (int k = 0; k < lead.size(); ++k) {
            m_leads[l].contacts.append(lead[k].second);
            if (k > 0)
                spacing = std::min(spacing, (m_contacts[lead[k].second].position - m_contacts[lead[k - 1].second].position).norm());
        }
        m_leads[l].type = spacing < kMaxDepthSpacing ? IeegLeadType::Depth : IeegLeadType::Strip;
    }
}

//=============================================================================================================

IeegImplantation IeegImplantation::readBids(const QString& electrodesTsv, QString* error)
{
    const QFileInfo info(electrodesTsv);
    // The coordsystem sidecar sharing the longest name prefix, so sessions are never mixed
    QString coordPath;
    int bestPrefix = -1;
    const QStringList candidates = info.dir().entryList({QStringLiteral("*_coordsystem.json")}, QDir::Files);
    for (const QString& candidate : candidates) {
        int prefix = 0;
        while (prefix < candidate.size() && prefix < info.fileName().size() && candidate[prefix] == info.fileName()[prefix])
            ++prefix;
        if (prefix > bestPrefix) {
            bestPrefix = prefix;
            coordPath = info.dir().filePath(candidate);
        }
    }
    if (coordPath.isEmpty()) {
        setError(error, QStringLiteral("no *_coordsystem.json next to %1, so the units are unknown").arg(electrodesTsv));
        return {};
    }
    const BIDSLIB::BidsCoordinateSystem coordSystem = BIDSLIB::BidsCoordinateSystem::readJson(coordPath);
    const float scale = coordSystem.units == QLatin1String("m") ? 1.0f
        : coordSystem.units == QLatin1String("cm")              ? 0.01f
        : coordSystem.units == QLatin1String("mm")              ? 0.001f
                                                                : 0.0f;
    if (scale == 0.0f) {
        setError(error, QStringLiteral("unsupported iEEGCoordinateUnits \"%1\" in %2").arg(coordSystem.units, coordPath));
        return {};
    }

    QVector<IeegContact> contacts;
    QHash<QString, QString> declaredTypes;
    QHash<QString, QString> declaredDimensions;
    for (const BIDSLIB::BidsElectrode& e : BIDSLIB::BidsElectrode::readTsv(electrodesTsv)) {
        bool okX = false, okY = false, okZ = false;
        const Eigen::Vector3f position(e.x.toFloat(&okX), e.y.toFloat(&okY), e.z.toFloat(&okZ));
        if (!okX || !okY || !okZ)
            continue;
        IeegContact contact;
        contact.name = e.name;
        contact.position = position * scale;
        contact.lead = e.group;
        contacts.append(contact);
        // Declarations are per lead; contacts without a group take their lead from the name later
        const QString key = e.group.isEmpty() ? e.name : e.group;
        if (!e.type.isEmpty())
            declaredTypes.insert(key, e.type.toLower());
        if (!e.dimension.isEmpty())
            declaredDimensions.insert(key, e.dimension);
    }
    if (contacts.isEmpty()) {
        setError(error, QStringLiteral("no contacts with coordinates in %1").arg(electrodesTsv));
        return {};
    }

    IeegImplantation implantation(contacts, coordSystem.system);
    static const QRegularExpression dimension(QStringLiteral("^\\[?\\s*(\\d+)\\s*x\\s*(\\d+)\\s*\\]?$"));
    for (IeegLead& lead : implantation.m_leads) {
        // A declaration on any contact of the lead applies to the lead
        QString type = declaredTypes.value(lead.name);
        QString shape = declaredDimensions.value(lead.name);
        for (const int c : std::as_const(lead.contacts)) {
            const QString& name = implantation.m_contacts[c].name;
            if (type.isEmpty())
                type = declaredTypes.value(name);
            if (shape.isEmpty())
                shape = declaredDimensions.value(name);
        }
        const QRegularExpressionMatch m = dimension.match(shape);
        if (m.hasMatch() && m.captured(1).toInt() * m.captured(2).toInt() == lead.contacts.size()) {
            lead.rows = m.captured(1).toInt();
            lead.cols = m.captured(2).toInt();
            if (lead.rows > 1 && lead.cols > 1)
                lead.type = IeegLeadType::Grid;
        }
        if (type.contains(QLatin1String("depth")) || type.contains(QLatin1String("seeg")))
            lead.type = IeegLeadType::Depth;
        else if (type.contains(QLatin1String("strip")))
            lead.type = IeegLeadType::Strip;
        else if (type.contains(QLatin1String("grid")))
            lead.type = IeegLeadType::Grid;
    }
    return implantation;
}

//=============================================================================================================

int IeegImplantation::indexOf(const QString& name) const
{
    for (int i = 0; i < m_contacts.size(); ++i) {
        if (m_contacts[i].name == name)
            return i;
    }
    return -1;
}

//=============================================================================================================

Eigen::MatrixX3f IeegImplantation::positions() const
{
    Eigen::MatrixX3f rr(m_contacts.size(), 3);
    for (int i = 0; i < m_contacts.size(); ++i)
        rr.row(i) = m_contacts[i].position.transpose();
    return rr;
}
