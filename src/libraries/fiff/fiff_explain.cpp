//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fiff_explain.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Implementation of @ref FIFFLIB::FiffExplain from the compiled-in fiff_explanations.txt table.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_explain.h"

#include <QFile>
#include <QHash>
#include <QRegularExpression>
#include <QTextStream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

// Q_INIT_RESOURCE must not be expanded inside a namespace; static builds need it.
static void initFiffExplanationsResource()
{
    Q_INIT_RESOURCE(fiff_explanations);
}

namespace
{

// Explanation classes of fiff_explanations.txt (MNE-C mne_show_fiff)
enum ExplanationClass
{
    TagClass = 1,
    BlockClass = 2,
    UnitClass = 3,
    UnitMultiplierClass = 4,
    ChannelKindClass = 5
};

quint64 key(int exclass, int kind)
{
    return (static_cast<quint64>(static_cast<quint32>(exclass)) << 32) | static_cast<quint32>(kind);
}

const QHash<quint64, QString>& explanations()
{
    static const QHash<quint64, QString> table = [] {
        initFiffExplanationsResource();
        QHash<quint64, QString> entries;
        QFile file(QStringLiteral(":/fiff/fiff_explanations.txt"));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning("[FiffExplain] the built-in explanation table is missing");
            return entries;
        }
        // <class> <kind> "<text>"; the first entry of a duplicated class/kind wins, inner spaces collapse as in MNE-C
        const QRegularExpression line(QStringLiteral(R"(^\s*(\d+)\s+(-?\d+)\s+"([^"]*)\")"));
        QTextStream in(&file);
        while (!in.atEnd()) {
            const QRegularExpressionMatch m = line.match(in.readLine());
            const quint64 k = m.hasMatch() ? key(m.captured(1).toInt(), m.captured(2).toInt()) : 0;
            if (m.hasMatch() && !entries.contains(k)) {
                entries.insert(k, m.captured(3).simplified());
            }
        }
        return entries;
    }();
    return table;
}

QString lookup(int exclass, int kind)
{
    return explanations().value(key(exclass, kind));
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

QString FiffExplain::tag(int kind)
{
    return lookup(TagClass, kind);
}

//=============================================================================================================

QString FiffExplain::block(int kind)
{
    return lookup(BlockClass, kind);
}

//=============================================================================================================

QString FiffExplain::unit(int unit)
{
    return lookup(UnitClass, unit);
}

//=============================================================================================================

QString FiffExplain::unitMultiplier(int unitMul)
{
    return lookup(UnitMultiplierClass, unitMul);
}

//=============================================================================================================

QString FiffExplain::channelKind(int kind)
{
    return lookup(ChannelKindClass, kind);
}
