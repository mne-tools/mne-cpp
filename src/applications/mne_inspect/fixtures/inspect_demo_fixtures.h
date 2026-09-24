//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inspect_demo_fixtures.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
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
