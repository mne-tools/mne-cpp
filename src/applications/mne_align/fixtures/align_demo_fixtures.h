//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     align_demo_fixtures.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 * @brief    Synthetic in-memory demo fixtures for the mne_align coregistration
 *           workflow. Used by `mne_doc_shots` to render the seven mne_align
 *           manual screenshots without any on-disk sample data.
 */

#ifndef MNEALIGN_ALIGN_DEMO_FIXTURES_H
#define MNEALIGN_ALIGN_DEMO_FIXTURES_H

#include <utils/polhemus/acquired_points.h>

#include <QString>
#include <QVector>

namespace MNEALIGN
{

//=============================================================================================================
/**
 * Build a synthetic NAS / LPA / RPA fiducial triplet expressed in the
 * Polhemus sensor frame (head-space metres). Coordinates are loosely
 * realistic for an adult head: NAS forward, LPA / RPA symmetric on
 * the ears.
 */
QVector<UTILSLIB::DigitizedPoint> demoFiducials();

//=============================================================================================================
/**
 * Build a synthetic EEG cap montage with the eight cardinal 10-20
 * positions (Fz, Cz, Pz, Oz, T7, T8, O1, O2) at plausible scalp
 * coordinates. The labels match the standard montage so the wizard's
 * cap pages stay self-consistent on screen.
 *
 * @param[in] count   Number of EEG points to return (clamped to 0..8).
 */
QVector<UTILSLIB::DigitizedPoint> demoEegCap(int count = 8);

//=============================================================================================================
/**
 * Build a synthetic head-shape point cloud sampled on the upper
 * hemisphere of a 100 mm sphere centred at the head origin.
 *
 * @param[in] count   Number of HSP points to return (>= 0).
 */
QVector<UTILSLIB::DigitizedPoint> demoHeadShape(int count = 40);

//=============================================================================================================
/**
 * Combined demo digitisation = three fiducials + full eight-electrode
 * EEG cap + 40 HSP points. Convenience helper used by the Verify / Save
 * / Done screenshots where every category should be populated.
 */
QVector<UTILSLIB::DigitizedPoint> demoFullDigitisation();

//=============================================================================================================
/**
 * Push @p points into @p store, replacing whatever was there before.
 * Only one @c pointsChanged signal is emitted (after the final point).
 */
void applyTo(UTILSLIB::AcquiredPoints* store,
             const QVector<UTILSLIB::DigitizedPoint>& points);

} // namespace MNEALIGN

#endif // MNEALIGN_ALIGN_DEMO_FIXTURES_H
