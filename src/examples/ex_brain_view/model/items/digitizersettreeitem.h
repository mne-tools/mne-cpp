//=============================================================================================================
/**
 * @file     digitizersettreeitem.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    2.0.0
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
 * @brief    DigitizerSetTreeItem class declaration.
 *
 */

#ifndef DIGITIZERSETTREEITEM_H
#define DIGITIZERSETTREEITEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

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
class DigitizerSetTreeItem : public QStandardItem
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
