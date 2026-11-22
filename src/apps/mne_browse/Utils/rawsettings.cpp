//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     rawsettings.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @date     January, 2014
 * @version  2.1.0
 * @brief    Contains all application settings.
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rawsettings.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBROWSE;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

QMap<int, QColor> RawSettings::eventTypeColors() const
{
    QMap<int, QColor> colors{{1, QColor(Qt::black)},
                             {2, QColor(Qt::magenta)},
                             {3, QColor(Qt::green)},
                             {4, QColor(Qt::red)},
                             {5, QColor(Qt::cyan)},
                             {32, QColor(Qt::yellow)},
                             {998, QColor(Qt::darkBlue)},
                             {999, QColor(Qt::darkCyan)}};

    const QString prefix = QStringLiteral("EventDesignParameters/event_color_");
    for (const QString& key : m_qSettings.allKeys()) {
        bool ok = false;
        const int iType = key.startsWith(prefix) ? key.mid(prefix.size()).toInt(&ok) : 0;
        if (ok) {
            colors[iType] = m_qSettings.value(key).value<QColor>();
        }
    }

    return colors;
}

//=============================================================================================================

void RawSettings::setEventTypeColor(int iType, const QColor& color)
{
    m_qSettings.setValue(QStringLiteral("EventDesignParameters/event_color_%1").arg(iType), color);
}

//=============================================================================================================

QColor RawSettings::dataMarkerColor() const
{
    return m_qSettings.value(QStringLiteral("DataMarker/data_marker_color"), QColor(93, 177, 47)).value<QColor>();
}
