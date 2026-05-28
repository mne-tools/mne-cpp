// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2012-2026 MNE-CPP Authors
//   Christoph Dinh <christoph.dinh@mne-cpp.org>
//   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
//   Gabriel Motta <gabrielbenmotta@gmail.com>

//=============================================================================================================
/**
 * @file     mri_cor_fif_io.h
  * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
 *
 * @brief    Writer for the COR.fif FIFF representation of an MRI volume (Neuromag/MNE convention used by all mne-c source-space tooling).
 *
 *           COR.fif is the canonical FIFF wrapper for MRI volume data used
 *           by every downstream mne-c step that needs MRI geometry ---
 *           coregistration, BEM surface generation, forward-model assembly,
 *           and the @c mne_analyze inverse pipeline all consume the same
 *           tag structure. Producing it here lets mne-cpp loaders
 *           (@ref MriMghIO, @ref MriNiftiIO, @ref MriCorIO) feed any
 *           legacy FIFF-driven workflow without requiring callers to detour
 *           through the MNE-Python @c mri.fif writer.
 *
 *           The writer takes an in-memory @ref MriVolData, materialises
 *           one @c FIFFB_MRI_SLICE block per slice (carrying the slice's
 *           geometry and pixel buffer in the source's native encoding ---
 *           byte / word / float, no quantisation), bundles them inside a
 *           single @c FIFFB_MRI_SET, and prefixes the head\u2192MRI identity
 *           plus any extra transforms the volume carries (e.g. Talairach).
 *
 *           Block structure:
 *
 *           FIFFB_MRI
 *             FIFF_BLOCK_ID
 *             FIFFB_MRI_SET
 *               FIFF_COORD_TRANS (HEAD -> MRI identity)
 *               [additional coord transforms: Talairach etc.]
 *               FIFFB_MRI_SLICE (repeated for each slice)
 *                 FIFF_MRI_WIDTH
 *                 FIFF_MRI_WIDTH_M
 *                 FIFF_MRI_HEIGHT
 *                 FIFF_MRI_HEIGHT_M
 *                 FIFF_COORD_TRANS (slice -> MRI)
 *                 FIFF_MRI_PIXEL_ENCODING
 *                 FIFF_MRI_PIXEL_DATA
 *               /FIFFB_MRI_SLICE
 *             /FIFFB_MRI_SET
 *           /FIFFB_MRI
 *
 *           Ported from save_slices() / write_slice() in MNE C write_mri_set.c
 *           by Matti Hamalainen.
 *
 */

#ifndef MRI_COR_FIF_IO_H
#define MRI_COR_FIF_IO_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mri_global.h"
#include "mri_vol_data.h"

#include <fiff/fiff_coord_trans.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QVector>

//=============================================================================================================
// DEFINE NAMESPACE MRILIB
//=============================================================================================================

namespace MRILIB {

//=============================================================================================================
/**
 * Writer for COR.fif FIFF files.
 *
 * Writes MRI slice data (COR or MGH-derived) into the FIFF format used by
 * the MNE C tools and MNE-Python for MRI volume representation.
 *
 * Ported from save_slices() / write_slice() in MNE C write_mri_set.c
 * by Matti Hamalainen.
 *
 * @brief COR.fif FIFF file writer.
 */
class MRISHARED_EXPORT MriCorFifIO
{
public:
    //=========================================================================================================
    /**
     * Writes COR.fif file from slice data.
     *
     * @param[in] fileName         Output FIFF file path.
     * @param[in] slices           Vector of MriSlice structures to write.
     * @param[in] additionalTrans  Additional coordinate transforms (Talairach etc.).
     *
     * @return True on success, false on error.
     */
    static bool write(const QString& fileName,
                      const QVector<MriSlice>& slices,
                      const QVector<FIFFLIB::FiffCoordTrans>& additionalTrans);
};

} // namespace MRILIB

#endif // MRI_COR_FIF_IO_H
