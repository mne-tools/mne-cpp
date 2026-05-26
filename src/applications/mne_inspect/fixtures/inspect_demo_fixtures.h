//=============================================================================================================
/**
 * @file     inspect_demo_fixtures.h
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
 * @brief    Synthetic in-memory demo fixtures for the mne_inspect multimodal
 *           workflow. Used by `mne_doc_shots` to render the four
 *           inspect-multimodal documentation screenshots without any
 *           on-disk sample data, and reused by `test_mne_inspect_multimodal`
 *           for headless smoke coverage.
 */

#ifndef MNEINSPECT_DEMO_FIXTURES_H
#define MNEINSPECT_DEMO_FIXTURES_H

#include <disp3D/renderable/electrodeobject.h>
#include <mri/mri_vol_data.h>

#include <QVector>

#include <memory>

namespace MNEINSPECT
{

//=============================================================================================================
/**
 * Build a small single-strip electrode fixture (one LA depth electrode with
 * four evenly spaced contacts along z). This matches the geometry the
 * test_mne_inspect_multimodal suite has used since v2.3.0 and is preserved
 * here so the test can keep its existing invariants while doc_shots uses
 * the richer multi-array fixture below.
 *
 * @return One `ElectrodeArray` with layout = Depth, label = "LA", four
 *         contacts at z = 0, 0.01, 0.02, 0.03 m.
 */
QVector<DISP3DLIB::ElectrodeArray> demoOneDepthStrip();

//=============================================================================================================
/**
 * Build the four-array demo electrode fixture used by the documentation
 * screenshots: two Depth (sEEG) strips on the left and right and two
 * Grid layouts (one Strip-like, one 4×4 ECoG grid). All contacts are
 * placed at plausible cm-scale offsets so they look like a realistic
 * intracranial montage when rendered in the empty multimodal scene.
 *
 * @return Four `ElectrodeArray`s: "LA" (Depth), "RA" (Depth), "LG"
 *         (Strip / 1×6), "RG" (Grid / 4×4).
 */
QVector<DISP3DLIB::ElectrodeArray> demoFourArrayMontage();

//=============================================================================================================
/**
 * Build a synthetic 3-D MRI volume entirely in memory. The volume is a
 * cubic float slab with a smooth Gaussian blob at its centre — enough
 * for the MriSlicesPlugin to materialise three ortho slices and for the
 * scene's slice renderable to draw something recognisable in screenshots
 * and headless tests, with zero filesystem dependencies.
 *
 * @param[in] dim  Side length in voxels of the cubic volume. Default 32.
 * @return         A freshly populated `MriVolData` ready to be handed to
 *                 `MRISLICESPLUGIN::MriSlicesPlugin::setVolume`.
 */
std::unique_ptr<MRILIB::MriVolData> demoMriSlab(int dim = 32);

}  // namespace MNEINSPECT

#endif  // MNEINSPECT_DEMO_FIXTURES_H
