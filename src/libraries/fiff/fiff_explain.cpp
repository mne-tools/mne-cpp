//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     fiff_explain.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October 2026
 * @brief    Implementation of @ref mne::FiffExplain from the compiled-in fiff_explanations.txt table.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_explain.h"
#include <base/log.h>

#include <string>
#include <unordered_map>

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>
#include <cstdint>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

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

std::uint64_t key(int exclass, int kind)
{
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(exclass)) << 32) | static_cast<std::uint32_t>(kind);
}

const std::unordered_map<std::uint64_t, std::string>& explanations()
{
    static const std::unordered_map<std::uint64_t, std::string> table = [] {
        initFiffExplanationsResource();
        std::unordered_map<std::uint64_t, std::string> entries;
        QFile file(QStringLiteral(":/fiff/fiff_explanations.txt"));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            mne::Log::warning("[FiffExplain] the built-in explanation table is missing");
            return entries;
        }
        // <class> <kind> "<text>"; the first entry of a duplicated class/kind wins, inner spaces collapse as in MNE-C
        const QRegularExpression line(QStringLiteral(R"(^\s*(\d+)\s+(-?\d+)\s+"([^"]*)\")"));
        QTextStream in(&file);
        while (!in.atEnd()) {
            const QRegularExpressionMatch m = line.match(in.readLine());
            const std::uint64_t k = m.hasMatch() ? key(m.captured(1).toInt(), m.captured(2).toInt()) : 0;
            if (m.hasMatch()) {
                entries.try_emplace(k, m.captured(3).simplified().toStdString());
            }
        }
        return entries;
    }();
    return table;
}

std::string lookup(int exclass, int kind)
{
    const auto& table = explanations();
    const auto it = table.find(key(exclass, kind));
    return it == table.end() ? std::string() : it->second;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

std::string FiffExplain::tag(int kind)
{
    return lookup(TagClass, kind);
}

//=============================================================================================================

std::string FiffExplain::block(int kind)
{
    return lookup(BlockClass, kind);
}

//=============================================================================================================

std::string FiffExplain::unit(int unit)
{
    return lookup(UnitClass, unit);
}

//=============================================================================================================

std::string FiffExplain::unitMultiplier(int unitMul)
{
    return lookup(UnitMultiplierClass, unitMul);
}

//=============================================================================================================

std::string FiffExplain::channelKind(int kind)
{
    return lookup(ChannelKindClass, kind);
}
