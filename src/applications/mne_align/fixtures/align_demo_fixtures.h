//=============================================================================================================
/**
 * @file     align_demo_fixtures.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided
 * that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and
 *       the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions
 *       and the following disclaimer in the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used to endorse or
 *       promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
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
