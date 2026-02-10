//=============================================================================================================
/**
 * @file     digitizersettreeitem.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     February, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    DigitizerSetTreeItem class implementation.
 *
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
