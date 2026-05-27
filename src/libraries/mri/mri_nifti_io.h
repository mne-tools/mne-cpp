//=============================================================================================================
/**
 * @file     mri_nifti_io.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
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
 * @brief    MriNiftiIO class declaration.
 *
 *           Reader for NIfTI-1 single-file volumes (.nii and .nii.gz).
 *
 *           Format reference: https://nifti.nimh.nih.gov/nifti-1
 *
 *           The header is 348 bytes; image data starts at @c vox_offset
 *           (typically 352 for single-file .nii). All scalar fields are
 *           little-endian by default; big-endian files are detected via the
 *           @c sizeof_hdr field. Voxel data is read into the same
 *           per-slice @ref MriSlice layout used by the MGH reader so that
 *           the downstream MRI slicing/rendering pipeline does not need to
 *           branch on the source format.
 *
 *           Transform extraction prefers @c sform (3 affine rows directly in
 *           voxel→RAS mm), falls back to @c qform (quaternion + offset), and
 *           finally to the @c pixdim diagonal centred at the volume origin.
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
 * Reader for NIfTI-1 single-file volumes.
 *
 * Populates an @ref MriVolData using the same per-slice layout that
 * @ref MriMghIO produces so that the rest of the rendering pipeline can
 * stay format-agnostic.
 *
 * @brief NIfTI-1 (.nii / .nii.gz) volume reader.
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
