//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     digitizersettreeitem.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    FIFF digitizer-point fan-out into per-category DigitizerTreeItem children with the canonical colour / radius scheme.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "digitizersettreeitem.h"

#include <mne/base/log.h>

#include "digitizertreeitem.h"
#include <mne/fiff/fiff_constants.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector3D>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <format>
#include <string>
#include <vector>

namespace mne::disp
{

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DigitizerSetTreeItem::DigitizerSetTreeItem(const QString& text,
                                           std::span<const mne::FiffDigPoint> digitizerPoints)
: QStandardItem(text)
{
    setCheckable(true);
    setCheckState(Qt::Checked);

    // Categorize points by kind
    // Cardinal points are further split by ident (Nasion, LPA, RPA) but grouped together
    std::vector<QVector3D> cardinalPos, hpiPos, eegPos, extraPos;
    std::vector<std::string> cardinalNames, hpiNames, eegNames, extraNames;

    int hpiIdx = 0, eegIdx = 0, extraIdx = 0;

    for (const auto& p : digitizerPoints) {
        QVector3D pos(p.r[0], p.r[1], p.r[2]);

        switch (p.kind) {
            case FIFFV_POINT_CARDINAL: {
                cardinalPos.push_back(pos);
                if (p.ident == FIFFV_POINT_NASION)
                    cardinalNames.push_back("Nasion");
                else if (p.ident == FIFFV_POINT_LPA)
                    cardinalNames.push_back("LPA");
                else if (p.ident == FIFFV_POINT_RPA)
                    cardinalNames.push_back("RPA");
                else
                    cardinalNames.push_back(std::format("Cardinal {}", p.ident));
                break;
            }
            case FIFFV_POINT_HPI:
                hpiPos.push_back(pos);
                hpiNames.push_back(std::format("HPI {}", ++hpiIdx));
                break;
            case FIFFV_POINT_EEG:
                eegPos.push_back(pos);
                eegNames.push_back(std::format("EEG {}", ++eegIdx));
                break;
            case FIFFV_POINT_EXTRA:
                extraPos.push_back(pos);
                extraNames.push_back(std::format("Extra {}", ++extraIdx));
                break;
            default:
                extraPos.push_back(pos);
                extraNames.push_back(std::format("Unknown {}", extraPos.size()));
                break;
        }
    }

    // Create child items for each non-empty category
    // Color and size scheme matches disp3D conventions
    if (!cardinalPos.empty()) {
        auto* item = new DigitizerTreeItem("Cardinal",
                                           DigitizerTreeItem::Cardinal,
                                           cardinalPos,
                                           cardinalNames,
                                           QColor(0, 255, 0), // Green
                                           0.002f);           // 2mm
        appendRow(item);
        mne::Log::debug("DigitizerSetTreeItem: Cardinal points: {}", cardinalPos.size());
    }

    if (!hpiPos.empty()) {
        auto* item = new DigitizerTreeItem("HPI",
                                           DigitizerTreeItem::HPI,
                                           hpiPos,
                                           hpiNames,
                                           QColor(128, 0, 0), // DarkRed
                                           0.003f);           // 3mm - enlarged for better visibility
        appendRow(item);
        mne::Log::debug("DigitizerSetTreeItem: HPI points: {}", hpiPos.size());
    }

    if (!eegPos.empty()) {
        auto* item = new DigitizerTreeItem("EEG",
                                           DigitizerTreeItem::EEG,
                                           eegPos,
                                           eegNames,
                                           QColor(0, 255, 255), // Cyan
                                           0.001f);             // 1mm
        appendRow(item);
        mne::Log::debug("DigitizerSetTreeItem: EEG dig points: {}", eegPos.size());
    }

    if (!extraPos.empty()) {
        auto* item = new DigitizerTreeItem("Extra",
                                           DigitizerTreeItem::Extra,
                                           extraPos,
                                           extraNames,
                                           QColor(255, 0, 255), // Magenta
                                           0.001f);             // 1mm
        appendRow(item);
        mne::Log::debug("DigitizerSetTreeItem: Extra/head shape points: {}", extraPos.size());
    }
}

//=============================================================================================================

DigitizerTreeItem* DigitizerSetTreeItem::categoryItem(int kind) const
{
    for (int i = 0; i < rowCount(); ++i) {
        DigitizerTreeItem* item = dynamic_cast<DigitizerTreeItem*>(child(i));
        if (item && static_cast<int>(item->pointKind()) == kind) {
            return item;
        }
    }
    return nullptr;
}

//=============================================================================================================

int DigitizerSetTreeItem::totalPointCount() const
{
    int count = 0;
    for (int i = 0; i < rowCount(); ++i) {
        DigitizerTreeItem* item = dynamic_cast<DigitizerTreeItem*>(child(i));
        if (item) {
            count += item->positions().size();
        }
    }
    return count;
}

} // namespace mne::disp
