//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mri_nifti_io.h
 * @since 2026
 * @date  May 2026
 * @brief NIfTI-1 single-file (.nii / .nii.gz) volume reader producing the same per-slice layout as the MGH reader.
 *
 * NIfTI-1 is the standard exchange container used by FSL, AFNI, SPM,
 * nibabel and essentially every neuroimaging toolchain outside the
 * FreeSurfer ecosystem; adding it to MRILIB lets mne-cpp consume
 * preprocessed structurals produced by any of those pipelines (e.g. BIDS
 * subjects whose anatomicals never pass through @c recon-all) without
 * requiring an external conversion step. The reader stays
 * surface-compatible with @ref MriMghIO so the slicing, rendering and
 * COR.fif export paths downstream do not need to branch on source format.
 *
 * The header is 348 bytes; image data starts at @c vox_offset (typically
 * 352 for single-file .nii). All scalar fields are little-endian by
 * default; big-endian files are detected via the @c sizeof_hdr magic and
 * the whole header is byte-swapped up-front. The @c .nii.gz variant is
 * decompressed in memory via zlib's @c MAX_WBITS+16 mode, mirroring the
 * MGZ path.
 *
 * Transform extraction follows the NIfTI-1 priority rule used by nibabel
 * and FSL: prefer @c sform (3 affine rows directly in voxel\u2192RAS mm),
 * fall back to @c qform (unit quaternion + offset, expanded into a
 * rotation matrix), and as a last resort build a diagonal transform from
 * @c pixdim centred at the volume origin. This ordering is encoded once
 * in @ref MriNiftiIO::read so every caller gets the same RAS regardless
 * of how the source file was authored.
 *
 * Format reference: https://nifti.nimh.nih.gov/nifti-1
 */

#ifndef MRI_NIFTI_IO_H
#define MRI_NIFTI_IO_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mri_global.h"
#include "mri_vol_data.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>

//=============================================================================================================
// DEFINE NAMESPACE MRILIB
//=============================================================================================================

namespace MRILIB {

//=============================================================================================================
/**
 * @brief Stateless decoder for NIfTI-1 single-file volumes (@c .nii and @c .nii.gz).
 *
 * Parses the 348-byte fixed-size header, expands any voxel-type / endianness
 * combination NIfTI-1 supports into the canonical 16-bit unsigned slice
 * buffer produced by @ref MriMghIO, and resolves voxel\u2192RAS via the
 * @c sform/qform/pixdim priority chain. The class is a pure namespace of
 * @c static methods so callers can use it without owning any state; the
 * populated @ref MriVolData carries the result.
 */
class MRISHARED_EXPORT MriNiftiIO
{
public:
    //=========================================================================================================
    /**
     * Reads a NIfTI-1 single-file volume.
     *
     * For @c .nii.gz inputs the file is decompressed in memory via zlib
     * (re-using the same @c MAX_WBITS+16 strategy as @ref MriMghIO).
     *
     * @param[in]  niiFile  Path to the @c .nii or @c .nii.gz file.
     * @param[out] volData  Volume to populate.
     * @param[in]  verbose  Print header geometry on success.
     *
     * @return True on success, false on parse / I/O error.
     */
    static bool read(const QString& niiFile, MriVolData& volData, bool verbose = false);

    //=========================================================================================================
    /**
     * Decompresses a @c .nii.gz file into @p rawData.
     *
     * Exposed for direct use by tests; @ref read calls it transparently
     * when the input path ends in @c .gz.
     *
     * @param[in]  gzFile   Path to the gzip-compressed input.
     * @param[out] rawData  Output buffer.
     *
     * @return True on success.
     */
    static bool decompress(const QString& gzFile, QByteArray& rawData);
};

} // namespace MRILIB

#endif // MRI_NIFTI_IO_H
