//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     digitizersettreeitem.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Container item that groups raw FIFF digitizer points by category (Cardinal, HPI, EEG, Extra).
 *
 * Takes a flat @c QList<FiffDigPoint> from the FIFF info block
 * and fans it into one @ref DigitizerTreeItem child per category
 * with the canonical colour scheme (Nasion green, LPA red, RPA
 * blue, HPI dark-red, EEG cyan, Extra magenta) and per-category
 * sphere radius (2 mm fiducials, 1 mm everything else).
 *
 * Children are rendered as a single batched-sphere mesh per
 * category so a 5000-point head shape is still one draw call.
 */

#ifndef DIGITIZERSETTREEITEM_H
#define DIGITIZERSETTREEITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp3D_global.h"

#include <QStandardItem>
#include <QList>

#include <fiff/fiff_dig_point.h>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class DigitizerTreeItem;

//=============================================================================================================
/**
 * DigitizerSetTreeItem is a container item that groups digitizer points by category.
 * It parses a list of FiffDigPoint entries and creates child DigitizerTreeItem instances
 * for each category: Cardinal (Nasion, LPA, RPA), HPI, EEG, and Extra (head shape).
 *
 * This matches the disp3D DigitizerSetTreeItem pattern, providing per-category
 * color coding, sizing, and independent visibility control.
 *
 * Category color scheme (matching disp3D):
 *   - Nasion:  Green   (0, 255, 0)   — 2mm
 *   - LPA:     Red     (255, 0, 0)   — 2mm
 *   - RPA:     Blue    (0, 0, 255)   — 2mm
 *   - HPI:     DarkRed (128, 0, 0)   — 1mm
 *   - EEG:     Cyan    (0, 255, 255) — 1mm
 *   - Extra:   Magenta (255, 0, 255) — 1mm
 *
 * @brief    Digitizer point set container tree item.
 */
class DISP3DSHARED_EXPORT DigitizerSetTreeItem : public QStandardItem
{
public:
    //=========================================================================================================
    /**
     * Constructs a DigitizerSetTreeItem from a list of FIFF digitizer points.
     *
     * @param[in] text           Display text (e.g. "Digitizer").
     * @param[in] digitizerPoints List of FIFF digitizer points to categorize.
     */
    explicit DigitizerSetTreeItem(const QString &text,
                                  const QList<FIFFLIB::FiffDigPoint> &digitizerPoints);
    ~DigitizerSetTreeItem() = default;

    //=========================================================================================================
    /**
     * Returns the child DigitizerTreeItem for a given category, or nullptr.
     *
     * @param[in] kind       The digitizer point category to find.
     * @return Pointer to the child item, or nullptr if category has no points.
     */
    DigitizerTreeItem* categoryItem(int kind) const;

    //=========================================================================================================
    /**
     * Returns the total number of digitizer points across all categories.
     *
     * @return Total point count.
     */
    int totalPointCount() const;
};

#endif // DIGITIZERSETTREEITEM_H
