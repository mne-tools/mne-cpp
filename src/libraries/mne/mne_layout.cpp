//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_layout.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Implementation of @ref MNELIB::MNELayout and @ref MNELIB::MNELayoutPort (MNE-C mne_layout.c).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_layout.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;

//=============================================================================================================
// DEFINE STATIC METHODS
//=============================================================================================================

namespace
{

bool isInside(const MNELayoutPort& port, const QRectF& area)
{
    return port.xmin >= area.left() && port.xmax <= area.right() && port.ymin >= area.top() && port.ymax <= area.bottom();
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

QString MNELayoutPort::cleanName(const QString& name)
{
    return name.section('-', 0, 0).remove(' ').toLower();
}

//=============================================================================================================

bool MNELayoutPort::contains(const QString& channel) const
{
    return names.contains(cleanName(channel));
}

//=============================================================================================================

std::optional<MNELayout> MNELayout::read(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("MNELayout::read - Cannot open %s", qPrintable(path));
        return std::nullopt;
    }

    MNELayout layout;
    layout.fileName = path;
    bool haveExtent = false;
    QTextStream in(&file);
    static const QRegularExpression whitespace(QStringLiteral("\\s+"));
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }
        QStringList fields = line.split(whitespace);
        if (!haveExtent) {
            bool ok[4];
            const double v[4] = {fields.value(0).toDouble(&ok[0]), fields.value(1).toDouble(&ok[1]), fields.value(2).toDouble(&ok[2]), fields.value(3).toDouble(&ok[3])};
            if (!(ok[0] && ok[1] && ok[2] && ok[3])) {
                qWarning("MNELayout::read - Illegal extent in %s", qPrintable(path));
                return std::nullopt;
            }
            layout.m_extent = QRectF(QPointF(v[0], v[2]), QPointF(v[1], v[3]));
            haveExtent = true;
            continue;
        }
        bool ok[5];
        MNELayoutPort port;
        port.portno = fields.value(0).toInt(&ok[0]);
        const double x0 = fields.value(1).toDouble(&ok[1]);
        const double y0 = fields.value(2).toDouble(&ok[2]);
        const double w = fields.value(3).toDouble(&ok[3]);
        const double h = fields.value(4).toDouble(&ok[4]);
        if (fields.size() < 5 || !(ok[0] && ok[1] && ok[2] && ok[3] && ok[4])) {
            qWarning("MNELayout::read - Bad layout item in %s: %s", qPrintable(path), qPrintable(line));
            return std::nullopt;
        }
        port.xmin = static_cast<float>(x0);
        port.xmax = static_cast<float>(x0 + std::fabs(w));
        port.ymin = static_cast<float>(y0);
        port.ymax = static_cast<float>(y0 + std::fabs(h));
        port.invert = h < 0;
        // The names are the rest of the line and may contain spaces ("MEG 0113:MEG 0112").
        port.label = line.section(whitespace, 5).trimmed();
        for (const QString& name : port.label.split(':', Qt::SkipEmptyParts)) {
            port.names << MNELayoutPort::cleanName(name);
        }
        layout.ports.push_back(port);
    }
    if (!haveExtent) {
        qWarning("MNELayout::read - No extent in %s", qPrintable(path));
        return std::nullopt;
    }
    layout.m_visible = layout.m_extent;
    return layout;
}

//=============================================================================================================

QMap<QString, QPointF> MNELayout::channelPositions() const
{
    QMap<QString, QPointF> positions;
    for (const MNELayoutPort& port : ports) {
        for (const QString& name : port.label.split(':', Qt::SkipEmptyParts)) {
            positions.insert(name.trimmed(), QPointF(port.xmin, port.ymin));
        }
    }
    return positions;
}

//=============================================================================================================

int MNELayout::matchPorts(const QString& channel)
{
    int nmatch = 0;
    for (MNELayoutPort& port : ports) {
        port.matched = !channel.isEmpty() && port.contains(channel) && isVisible(port);
        nmatch += port.matched ? 1 : 0;
    }
    return nmatch;
}

//=============================================================================================================

int MNELayout::matchPorts(const QStringList& channels)
{
    m_matches = Eigen::MatrixXi::Zero(static_cast<Eigen::Index>(ports.size()), channels.size());
    int nmatch = 0;
    for (std::size_t p = 0; p < ports.size(); ++p) {
        MNELayoutPort& port = ports[p];
        port.matched = false;
        if (!isVisible(port)) {
            continue;
        }
        for (int c = 0; c < channels.size(); ++c) {
            if (port.contains(channels[c])) {
                m_matches(static_cast<Eigen::Index>(p), c) = 1;
                port.matched = true;
                ++nmatch;
            }
        }
    }
    return nmatch;
}

//=============================================================================================================

int MNELayout::portAt(const QPointF& point) const
{
    const auto it = std::find_if(ports.cbegin(), ports.cend(), [&point](const MNELayoutPort& port) {
        return point.x() > port.xmin && point.x() < port.xmax && point.y() > port.ymin && point.y() < port.ymax;
    });
    return it == ports.cend() ? -1 : static_cast<int>(it - ports.cbegin());
}

//=============================================================================================================

int MNELayout::confine(const QRectF& area)
{
    const QRectF requested = area.normalized();
    QRectF covered;
    int nmatch = 0;
    for (const MNELayoutPort& port : ports) {
        if (isInside(port, requested)) {
            covered = nmatch == 0 ? port.rect() : covered.united(port.rect());
            ++nmatch;
        }
    }
    if (nmatch > 0) {
        const double margin = 0.01 * covered.width();
        m_visible = covered.adjusted(-margin, -margin, margin, margin);
    }
    return nmatch;
}

//=============================================================================================================

bool MNELayout::isVisible(const MNELayoutPort& port) const
{
    return isInside(port, m_visible);
}
