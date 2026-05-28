//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file digitizersettreeitem.cpp
 * @since March 2026
 * @brief FIFF digitizer-point fan-out into per-category DigitizerTreeItem children with the canonical colour / radius scheme.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "digitizersettreeitem.h"
#include "digitizertreeitem.h"

#include <fiff/fiff_constants.h>

#include <QVector3D>
#include <QDebug>

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DigitizerSetTreeItem::DigitizerSetTreeItem(const QString &text,
                                           const QList<FIFFLIB::FiffDigPoint> &digitizerPoints)
    : QStandardItem(text)
{
    setCheckable(true);
    setCheckState(Qt::Checked);

    // Categorize points by kind
    // Cardinal points are further split by ident (Nasion, LPA, RPA) but grouped together
    QVector<QVector3D> cardinalPos, hpiPos, eegPos, extraPos;
    QStringList cardinalNames, hpiNames, eegNames, extraNames;

    int hpiIdx = 0, eegIdx = 0, extraIdx = 0;

    for (const auto &p : digitizerPoints) {
        QVector3D pos(p.r[0], p.r[1], p.r[2]);

        switch (p.kind) {
        case FIFFV_POINT_CARDINAL: {
            cardinalPos.append(pos);
            if (p.ident == FIFFV_POINT_NASION)
                cardinalNames.append("Nasion");
            else if (p.ident == FIFFV_POINT_LPA)
                cardinalNames.append("LPA");
            else if (p.ident == FIFFV_POINT_RPA)
                cardinalNames.append("RPA");
            else
                cardinalNames.append(QString("Cardinal %1").arg(p.ident));
            break;
        }
        case FIFFV_POINT_HPI:
            hpiPos.append(pos);
            hpiNames.append(QString("HPI %1").arg(++hpiIdx));
            break;
        case FIFFV_POINT_EEG:
            eegPos.append(pos);
            eegNames.append(QString("EEG %1").arg(++eegIdx));
            break;
        case FIFFV_POINT_EXTRA:
            extraPos.append(pos);
            extraNames.append(QString("Extra %1").arg(++extraIdx));
            break;
        default:
            extraPos.append(pos);
            extraNames.append(QString("Unknown %1").arg(extraPos.size()));
            break;
        }
    }

    // Create child items for each non-empty category
    // Color and size scheme matches disp3D conventions
    if (!cardinalPos.isEmpty()) {
        auto *item = new DigitizerTreeItem("Cardinal",
                                           DigitizerTreeItem::Cardinal,
                                           cardinalPos,
                                           cardinalNames,
                                           QColor(0, 255, 0),   // Green
                                           0.002f);             // 2mm
        appendRow(item);
        qDebug() << "DigitizerSetTreeItem: Cardinal points:" << cardinalPos.size();
    }

    if (!hpiPos.isEmpty()) {
        auto *item = new DigitizerTreeItem("HPI",
                                           DigitizerTreeItem::HPI,
                                           hpiPos,
                                           hpiNames,
                                           QColor(128, 0, 0),   // DarkRed
                                           0.001f);             // 1mm
        appendRow(item);
        qDebug() << "DigitizerSetTreeItem: HPI points:" << hpiPos.size();
    }

    if (!eegPos.isEmpty()) {
        auto *item = new DigitizerTreeItem("EEG",
                                           DigitizerTreeItem::EEG,
                                           eegPos,
                                           eegNames,
                                           QColor(0, 255, 255), // Cyan
                                           0.001f);             // 1mm
        appendRow(item);
        qDebug() << "DigitizerSetTreeItem: EEG dig points:" << eegPos.size();
    }

    if (!extraPos.isEmpty()) {
        auto *item = new DigitizerTreeItem("Extra",
                                           DigitizerTreeItem::Extra,
                                           extraPos,
                                           extraNames,
                                           QColor(255, 0, 255), // Magenta
                                           0.001f);             // 1mm
        appendRow(item);
        qDebug() << "DigitizerSetTreeItem: Extra/head shape points:" << extraPos.size();
    }
}

//=============================================================================================================

DigitizerTreeItem* DigitizerSetTreeItem::categoryItem(int kind) const
{
    for (int i = 0; i < rowCount(); ++i) {
        DigitizerTreeItem *item = dynamic_cast<DigitizerTreeItem*>(child(i));
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
        DigitizerTreeItem *item = dynamic_cast<DigitizerTreeItem*>(child(i));
        if (item) {
            count += item->positions().size();
        }
    }
    return count;
}
